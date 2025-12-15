#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <cstdlib>
#include <ctime>
#include <sys/time.h>

#define START_PORT 9501
#define NUM_PORTS 16
#define BUF_SIZE 512
#define MAX_EVENTS 16

using namespace std;

int epfd;
int sockets[NUM_PORTS];
bool epollActive[NUM_PORTS];
long nextToggleTimeMs[NUM_PORTS];

pthread_mutex_t epollMutex = PTHREAD_MUTEX_INITIALIZER;

long getTimeMs()
{
    timeval tv;
    gettimeofday(&tv, nullptr);
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

int randomSec(int min, int max)
{
    return (rand() % (max - min + 1)) + min;
}

void ePollAdd(int fd)
{
    pthread_mutex_lock(&epollMutex);

    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);

    pthread_mutex_unlock(&epollMutex);
}

void ePollDel(int fd)
{
    pthread_mutex_lock(&epollMutex);

    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);

    pthread_mutex_unlock(&epollMutex);
}

void* schedulerThread(void*)
{
    srand(time(nullptr) ^ pthread_self());

    long now = getTimeMs();

    for (int i = 0; i < NUM_PORTS; i++)
    {
        int activeTime = randomSec(2, 7);
        nextToggleTimeMs[i] = now + (activeTime * 1000);
    }

    while (true)
    {
        now = getTimeMs();

        for (int i = 0; i < NUM_PORTS; i++)
        {
            if (now >= nextToggleTimeMs[i])
            {
                if (epollActive[i])
                {
                    ePollDel(sockets[i]);
                    epollActive[i] = false;

                    int inactiveTime = randomSec(2, 7);
                    nextToggleTimeMs[i] = now + (inactiveTime * 1000);

                    cout << "EPOLL DEL port " << (START_PORT + i)
                         << " inactive " << inactiveTime << " sec" << endl;
                }
                else
                {
                    ePollAdd(sockets[i]);
                    epollActive[i] = true;

                    int activeTime = randomSec(2, 7);
                    nextToggleTimeMs[i] = now + (activeTime * 1000);

                    cout << "EPOLL ADD port " << (START_PORT + i)
                         << " active " << activeTime << " sec" << endl;
                }
            }
        }

        usleep(10000);
    }

    return nullptr;
}

int main()
{
    epfd = epoll_create1(0);

    for (int i = 0; i < NUM_PORTS; i++)
    {
        int port = START_PORT + i;

        int sockfd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);

        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;

        bind(sockfd, (sockaddr*)&addr, sizeof(addr));

        sockets[i] = sockfd;
        epollActive[i] = true;

        //ePollAdd(sockfd);

        cout << "Listening on port " << port << endl;
    }

    pthread_t tid;
    pthread_create(&tid, nullptr, schedulerThread, nullptr);

    epoll_event events[MAX_EVENTS];

    while (true)
    {
        int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);

        for (int i = 0; i < nfds; i++)
        {
            int fd = events[i].data.fd;

            while (true)
            {
                char buffer[BUF_SIZE];
                sockaddr_in src;
                socklen_t srclen = sizeof(src);

                ssize_t len = recvfrom(fd, buffer, BUF_SIZE - 1, 0,
                                       (sockaddr*)&src, &srclen);

                if (len < 0)
                {
                    break;
                }

                buffer[len] = '\0';

                sockaddr_in local;
                socklen_t locallen = sizeof(local);
                getsockname(fd, (sockaddr*)&local, &locallen);

                cout << "RX port " << ntohs(local.sin_port)
                     << " data: " << buffer << endl;
            }
        }
    }

    return 0;
}
