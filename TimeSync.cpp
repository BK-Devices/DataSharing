// timesync_ts.cpp
// Compile:
//   g++ -O2 -std=c++17 timesync_ts.cpp -o timesync_ts -pthread
//
// Usage examples:
//   # responder only (no sending mode): listens and replies to requests
//   sudo ./timesync_ts
//
//   # mode 0: send->print differences (peer IP required)
//   sudo ./timesync_ts 192.168.1.11 0
//
//   # mode 1: send->apply adjtime correction (peer IP required; requires root/CAP_SYS_TIME)
//   sudo ./timesync_ts 192.168.1.11 1
//
// Notes:
//  - Uses UDP port 12345 by default. Change PORT if needed.
//  - Uses SO_TIMESTAMPING to read RX timestamps (software/hardware) from kernel when available.
//  - TX timestamps are taken via clock_gettime(CLOCK_REALTIME) immediately before send
//    and are included in the packet. For ultimate accuracy you can extend code to read
//    TX timestamps from the error queue (advanced) if your NIC/driver supports it.
//  - Run as root for best results (CAP_NET_ADMIN for timestamping options and CAP_SYS_TIME to adjust time).
//  - Real zero-time-difference cannot be guaranteed; network hardware and load affect accuracy.

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/net_tstamp.h>
#include <linux/sockios.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sched.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <time.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstring>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

constexpr uint16_t PORT = 12345;
constexpr size_t BUFSZ = 512;
static std::atomic<bool> keep_running{true};

void sigint_handler(int) { keep_running = false; }

inline uint64_t timespec_to_us(const struct timespec &ts) {
    return uint64_t(ts.tv_sec) * 1000000ULL + uint64_t(ts.tv_nsec / 1000ULL);
}
inline void us_to_timeval(int64_t us, struct timeval &tv) {
    tv.tv_sec = us / 1000000LL;
    tv.tv_usec = us % 1000000LL;
}

// Simple htonll/ntohll
static inline uint64_t htonll(uint64_t x) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return (((uint64_t)htonl((uint32_t)(x & 0xffffffffULL))) << 32) | htonl((uint32_t)(x >> 32));
#else
    return x;
#endif
}
static inline uint64_t ntohll(uint64_t x) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return (((uint64_t)ntohl((uint32_t)(x & 0xffffffffULL))) << 32) | ntohl((uint32_t)(x >> 32));
#else
    return x;
#endif
}

// Packet layout (fixed-size)
struct Packet {
    uint32_t seq;
    uint8_t type; // 1=request, 2=reply
    uint8_t pad[3];
    uint64_t t1; // originate (requester) - us since epoch
    uint64_t t2; // receive at responder - us
    uint64_t t3; // transmit at responder - us
};

void pack_packet(const Packet &p, std::vector<uint8_t> &buf) {
    buf.resize(sizeof(Packet));
    uint32_t s = htonl(p.seq);
    uint64_t a = htonll(p.t1);
    uint64_t b = htonll(p.t2);
    uint64_t c = htonll(p.t3);
    uint8_t *ptr = buf.data();
    memcpy(ptr, &s, 4); ptr += 4;
    memcpy(ptr, &p.type, 1); ptr += 1;
    memset(ptr, 0, 3); ptr += 3;
    memcpy(ptr, &a, 8); ptr += 8;
    memcpy(ptr, &b, 8); ptr += 8;
    memcpy(ptr, &c, 8);
}

bool unpack_packet(const uint8_t *buf, size_t len, Packet &p) {
    if (len < sizeof(Packet)) return false;
    const uint8_t *ptr = buf;
    uint32_t s;
    memcpy(&s, ptr, 4); ptr += 4;
    p.seq = ntohl(s);
    memcpy(&p.type, ptr, 1); ptr += 1;
    ptr += 3;
    uint64_t a,b,c;
    memcpy(&a, ptr, 8); ptr += 8;
    memcpy(&b, ptr, 8); ptr += 8;
    memcpy(&c, ptr, 8);
    p.t1 = ntohll(a);
    p.t2 = ntohll(b);
    p.t3 = ntohll(c);
    return true;
}

// set realtime priority for a std::thread
bool set_thread_realtime(std::thread &th, int priority = 80) {
    struct sched_param param;
    param.sched_priority = priority;
    int policy = SCHED_FIFO;
    int ret = pthread_setschedparam(th.native_handle(), policy, &param);
    if (ret != 0) {
        std::cerr << "Warning: failed to set SCHED_FIFO on thread: " << strerror(ret) << "\n";
        return false;
    }
    return true;
}

// enable SO_TIMESTAMPING on socket
bool enable_timestamping(int sock) {
    // request RX timestamps (hardware + software), and raw hardware
    int flags = SOF_TIMESTAMPING_RX_SOFTWARE | SOF_TIMESTAMPING_RX_HARDWARE |
                SOF_TIMESTAMPING_RAW_HARDWARE | SOF_TIMESTAMPING_SOFTWARE;
    if (setsockopt(sock, SOL_SOCKET, SO_TIMESTAMPING, &flags, sizeof(flags)) < 0) {
        perror("setsockopt(SO_TIMESTAMPING)");
        // Not fatal: we will still fall back to user-space timestamps but warn
        return false;
    }
    return true;
}

// parse SCM_TIMESTAMPING control message to extract best timestamp (prefer hardware raw -> software)
bool extract_timestamp_from_cmsg(struct msghdr &msg, struct timespec &out_ts) {
    out_ts.tv_sec = 0;
    out_ts.tv_nsec = 0;
    for (struct cmsghdr *c = CMSG_FIRSTHDR(&msg); c; c = CMSG_NXTHDR(&msg, c)) {
        if (c->cmsg_level == SOL_SOCKET && c->cmsg_type == SO_TIMESTAMPING) {
            // c->cmsg_len should contain 3 * sizeof(timespec)
            struct timespec *ts = (struct timespec *)CMSG_DATA(c);
            // Order (kernel): [0] = software, [1] = hardware, [2] = raw hardware (driver)
            // We prefer raw hardware, then hardware, then software.
            // But availability depends on NIC & driver.
            if (ts[2].tv_sec != 0 || ts[2].tv_nsec != 0) {
                out_ts = ts[2];
                return true;
            } else if (ts[1].tv_sec != 0 || ts[1].tv_nsec != 0) {
                out_ts = ts[1];
                return true;
            } else if (ts[0].tv_sec != 0 || ts[0].tv_nsec != 0) {
                out_ts = ts[0];
                return true;
            }
        }
    }
    return false;
}

// get CLOCK_REALTIME in microseconds
uint64_t clock_realtime_us() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return timespec_to_us(ts);
}

// receiver thread: listens for incoming request packets and replies
void receiver_thread_fn(int sock) {
    // increase priority locally using pthread (this thread's handle)
    try {
        std::thread::id id = std::this_thread::get_id();
        // cannot set via std::thread here; but main thread already set for thread object
    } catch(...) {}

    uint8_t buf[BUFSZ];
    struct sockaddr_in peer_addr;
    socklen_t peer_len = sizeof(peer_addr);

    while (keep_running.load()) {
        struct iovec iov;
        iov.iov_base = buf;
        iov.iov_len = sizeof(buf);

        // prepare control buffer for timestamping
        char ctrl[512];
        struct msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_name = &peer_addr;
        msg.msg_namelen = sizeof(peer_addr);
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        msg.msg_control = ctrl;
        msg.msg_controllen = sizeof(ctrl);

        ssize_t rec = recvmsg(sock, &msg, 0);
        if (rec < 0) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
            perror("recvmsg");
            continue;
        }

        Packet p{};
        if (!unpack_packet(buf, (size_t)rec, p)) {
            std::cerr << "Received malformed packet\n";
            continue;
        }

        // extract kernel timestamp (if present)
        struct timespec kts{};
        bool have_kts = extract_timestamp_from_cmsg(msg, kts);
        uint64_t t_recv_us;
        if (have_kts) {
            t_recv_us = timespec_to_us(kts);
        } else {
            // fallback to userland clock
            t_recv_us = clock_realtime_us();
        }

        char peer_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &peer_addr.sin_addr, peer_ip, sizeof(peer_ip));

        if (p.type == 1) {
            // It's a request. Build reply containing their T1 and our T2/T3.
            Packet reply{};
            reply.seq = p.seq;
            reply.type = 2;
            reply.t1 = p.t1;
            reply.t2 = t_recv_us;
            // prepare reply buffer
            std::vector<uint8_t> rb;
            reply.t3 = 0; // will set to tx time just before send
            pack_packet(reply, rb);

            // set t3 = current realtime clock just before send
            uint64_t t3 = clock_realtime_us();
            reply.t3 = t3;
            uint64_t t3_net = htonll(reply.t3);
            // t3 offset in packet: 4 +1 +3 +8 +8 = 24
            memcpy(rb.data() + 24, &t3_net, 8);
            // also ensure t2 is packed (it was earlier) - but we packed earlier; update to be safe
            uint64_t t2_net = htonll(reply.t2);
            memcpy(rb.data() + 16, &t2_net, 8);

            ssize_t s = sendto(sock, rb.data(), rb.size(), 0,
                               (struct sockaddr*)&peer_addr, peer_len);
            if (s < 0) {
                perror("sendto(reply)");
            } else {
                std::cout << "Replied to " << peer_ip << " seq=" << reply.seq
                          << " T2=" << reply.t2 << " T3=" << reply.t3 << "\n";
            }
        } else if (p.type == 2) {
            // We received a reply: user-mode behavior handled in sender thread usually.
            // For robustness, print notice
            std::cout << "Received unsolicited reply from " << peer_ip << " seq=" << p.seq << "\n";
        } else {
            std::cerr << "Unknown packet type " << int(p.type) << " from " << peer_ip << "\n";
        }
    }
}

// sender thread: sends requests periodically and handles replies and error queue timestamps if desired
void sender_thread_fn(int sock, struct sockaddr_in peer, int mode) {
    uint32_t seq = 1;
    uint8_t outbuf[BUFSZ];

    // set recv timeout so recvmsg won't block forever
    struct timeval rto{1,0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &rto, sizeof(rto));

    while (keep_running.load()) {
        // Prepare request packet
        Packet req{};
        req.seq = seq;
        req.type = 1;
        req.t1 = clock_realtime_us(); // originate timestamp from userland right before send
        req.t2 = 0;
        req.t3 = 0;
        std::vector<uint8_t> sb;
        pack_packet(req, sb);

        // send
        ssize_t s = sendto(sock, sb.data(), sb.size(), 0,
                           (struct sockaddr*)&peer, sizeof(peer));
        if (s < 0) {
            perror("sendto(request)");
            // keep trying next iteration
        }

        // Now wait for reply (recvmsg) and capture kernel timestamp for arrival (T4)
        uint8_t rbuf[BUFSZ];
        struct iovec iov;
        iov.iov_base = rbuf;
        iov.iov_len = sizeof(rbuf);

        char ctrl[512];
        struct msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_name = nullptr;
        msg.msg_namelen = 0;
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        msg.msg_control = ctrl;
        msg.msg_controllen = sizeof(ctrl);

        ssize_t rec = recvmsg(sock, &msg, 0);
        uint64_t T4;
        if (rec < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                std::cerr << "No reply for seq=" << seq << " (timeout)\n";
                seq++;
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            } else {
                perror("recvmsg");
                seq++;
                continue;
            }
        } else {
            struct timespec kts;
            bool have_kts = extract_timestamp_from_cmsg(msg, kts);
            if (have_kts) {
                T4 = timespec_to_us(kts);
            } else {
                T4 = clock_realtime_us();
            }
        }

        Packet p;
        if (!unpack_packet(rbuf, (size_t)rec, p)) {
            std::cerr << "Malformed reply\n";
            seq++;
            continue;
        }

        // we have T1 (p.t1), T2 (p.t2), T3 (p.t3), T4 (arrival)
        uint64_t T1 = p.t1; // this is echo of our T1 (we used userland t1 when sending)
        uint64_t T2 = p.t2;
        uint64_t T3 = p.t3;
        uint64_t T4_local = T4;

        // compute offset and delay using standard NTP formula
        // offset = ((T2 - T1) + (T3 - T4)) / 2
        int64_t offset = ((int64_t)T2 - (int64_t)T1 + (int64_t)T3 - (int64_t)T4_local) / 2;
        int64_t delay  = ((int64_t)T4_local - (int64_t)T1) - ((int64_t)T3 - (int64_t)T2);

        std::cout << "seq=" << seq
                  << " T1=" << T1 << " T2=" << T2
                  << " T3=" << T3 << " T4=" << T4_local << "\n";
        std::cout << "  offset(us)=" << offset << "  rtt(us)=" << delay << "\n";

        if (mode == 0) {
            // just print differences (already printed)
        } else if (mode == 1) {
            // apply correction with adjtime (smooth)
            if (offset == 0) {
                std::cout << "  offset zero: no adjustment\n";
            } else {
                struct timeval adjust;
                adjust.tv_sec = offset / 1000000LL;
                adjust.tv_usec = offset % 1000000LL;
                if (adjust.tv_usec < 0) {
                    adjust.tv_usec += 1000000;
                    adjust.tv_sec -= 1;
                }
                struct timeval old{};
                int er = adjtime(&adjust, &old);
                if (er == 0) {
                    std::cout << "  adjtime called: adjust " << adjust.tv_sec
                              << "s " << adjust.tv_usec << "us\n";
                    if (old.tv_sec != 0 || old.tv_usec != 0) {
                        std::cout << "  previously pending adj: " << old.tv_sec
                                  << "s " << old.tv_usec << "us\n";
                    }
                } else {
                    perror("adjtime");
                    std::cerr << "  (need root / CAP_SYS_TIME)\n";
                }
            }
        }

        seq++;
        // pace between requests
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

int main(int argc, char **argv) {
    signal(SIGINT, sigint_handler);

    std::string peer_ip;
    bool have_mode = false;
    int mode = -1; // -1 = responder-only
    if (argc >= 2) {
        // first arg could be IP or mode; we interpret as: ./prog [peer_ip] [mode]
        peer_ip = argv[1];
        if (argc >= 3) {
            have_mode = true;
            mode = atoi(argv[2]);
        } else {
            // if only one arg and it's numeric? We'll assume it's IP; require mode if sending.
        }
    }

    // If peer_ip empty => responder-only mode
    bool will_send = (peer_ip.size() > 0 && have_mode && (mode == 0 || mode == 1));

    // Create UDP socket
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    // bind
    struct sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port = htons(PORT);
    if (bind(sock, (struct sockaddr*)&local, sizeof(local)) < 0) {
        perror("bind");
        close(sock);
        return 1;
    }

    // enable timestamping (best-effort)
    bool ts_ok = enable_timestamping(sock);
    if (!ts_ok) {
        std::cerr << "Warning: SO_TIMESTAMPING setup failed or partially unsupported. Kernel RX timestamps may be unavailable.\n";
    } else {
        std::cerr << "SO_TIMESTAMPING enabled (if supported by kernel/NIC/driver).\n";
    }

    // make socket non-blocking so threads remain responsive
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    // prepare peer sockaddr if needed
    struct sockaddr_in peer{};
    memset(&peer, 0, sizeof(peer));
    if (will_send) {
        peer.sin_family = AF_INET;
        peer.sin_port = htons(PORT);
        if (inet_aton(peer_ip.c_str(), &peer.sin_addr) == 0) {
            std::cerr << "Invalid peer IP: " << peer_ip << "\n";
            close(sock);
            return 1;
        }
    }

    std::cout << "timesync_ts started. port=" << PORT
              << " send=" << (will_send ? "yes" : "no")
              << " mode=" << (have_mode ? std::to_string(mode) : std::string("responder-only"))
              << "\n";
    std::cout << "Note: mode 1 (apply time) requires root or CAP_SYS_TIME.\n";

    // Launch receiver thread (always)
    std::thread receiver_thread(receiver_thread_fn, sock);

    // Try set high priority for receiver thread
    if (!set_thread_realtime(receiver_thread, 80)) {
        std::cerr << "Warning: failed to set realtime priority for receiver thread\n";
    }

    std::thread sender_thread;
    if (will_send) {
        sender_thread = std::thread(sender_thread_fn, sock, peer, mode);
        if (!set_thread_realtime(sender_thread, 80)) {
            std::cerr << "Warning: failed to set realtime priority for sender thread\n";
        }
    }

    // Main thread waits until interrupted
    while (keep_running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    // join threads and cleanup
    if (will_send) {
        if (sender_thread.joinable()) sender_thread.join();
    }
    if (receiver_thread.joinable()) receiver_thread.join();

    close(sock);
    std::cout << "Exiting.\n";
    return 0;
}

