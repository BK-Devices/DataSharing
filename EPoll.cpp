#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>

static int make_socket_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int create_udp_socket(int port)
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (make_socket_nonblocking(sock) < 0) {
        perror("nonblock");
        exit(1);
    }

    return sock;
}

void process_packet(int socket_id, const char* data, ssize_t len)
{
    std::cout << "[Socket " << socket_id << "] Received: "
              << std::string(data, len) << "\n";
}

int main()
{
    // Three sockets on three different ports
    int sock1 = create_udp_socket(5001);
    int sock2 = create_udp_socket(5002);
    int sock3 = create_udp_socket(5003);

    struct pollfd fds[3];
    fds[0].fd = sock1; fds[0].events = POLLIN;
    fds[1].fd = sock2; fds[1].events = POLLIN;
    fds[2].fd = sock3; fds[2].events = POLLIN;

    char buffer[2048];

    while (true)
    {
        // Timeout: 1 ms = 1
        int ret = poll(fds, 3, 1);

        if (ret < 0) {
            perror("poll");
            continue;
        }

        // Check each socket
        for (int i = 0; i < 3; i++)
        {
            if (fds[i].revents & POLLIN)
            {
                sockaddr_in src{};
                socklen_t sl = sizeof(src);

                ssize_t len = recvfrom(fds[i].fd, buffer, sizeof(buffer), 0,
                                       (sockaddr*)&src, &sl);
                if (len > 0) {
                    process_packet(i + 1, buffer, len);
                }
            }
        }

        // Sleep small amount to avoid busy looping
        std::this_thread::sleep_for(std::chrono::microseconds(200));
    }

    close(sock1);
    close(sock2);
    close(sock3);
    return 0;
}

