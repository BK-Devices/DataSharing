#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <cstdlib>
#include <ctime>
#include <sys/time.h>
#include <cstring>

#define DEST_IP "127.0.0.1"
#define START_PORT 9501
#define NUM_PORTS 16
#define BUF_SIZE 256

using namespace std;

int sockfd;
sockaddr_in dest[NUM_PORTS];

volatile bool sendFlag[NUM_PORTS];
long nextSendTimeMs[NUM_PORTS];

pthread_mutex_t flagMutex = PTHREAD_MUTEX_INITIALIZER;

long getTimeMs()
{
    timeval tv;
    gettimeofday(&tv, nullptr);
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

void* schedulerThread(void*)
{
    srand(time(nullptr));

    for (int i = 0; i < NUM_PORTS; i++)
    {
        int delay = (rand() % 801) + 200;
        nextSendTimeMs[i] = getTimeMs() + delay;
    }

    while (true)
    {
        long now = getTimeMs();

        for (int i = 0; i < NUM_PORTS; i++)
        {
            if (now >= nextSendTimeMs[i])
            {
                pthread_mutex_lock(&flagMutex);

                sendFlag[i] = true;

                int delay = (rand() % 801) + 200;
                nextSendTimeMs[i] = now + delay;

                pthread_mutex_unlock(&flagMutex);
            }
        }

        usleep(1000);
    }

    return nullptr;
}

void* senderThread(void*)
{
    int counter[NUM_PORTS];
    memset(counter, 0, sizeof(counter));

    while (true)
    {
        for (int i = 0; i < NUM_PORTS; i++)
        {
            if (sendFlag[i])
            {
                pthread_mutex_lock(&flagMutex);

                if (sendFlag[i])
                {
                    sendFlag[i] = false;

                    char buffer[BUF_SIZE];
                    snprintf(buffer, sizeof(buffer), "MSG %d PORT %d", counter[i], START_PORT + i);

                    sendto(sockfd, buffer, strlen(buffer), 0, (sockaddr*)&dest[i], sizeof(dest[i]));

                    std::cout << "TX port " << (START_PORT + i) << std::endl;

                    counter[i]++;
                }

                pthread_mutex_unlock(&flagMutex);
            }
        }

        usleep(1000);
    }

    return nullptr;
}

int main()
{
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    for (int i = 0; i < NUM_PORTS; i++)
    {
        dest[i].sin_family = AF_INET;
        dest[i].sin_port = htons(START_PORT + i);
        inet_pton(AF_INET, DEST_IP, &dest[i].sin_addr);

        sendFlag[i] = false;
    }

    pthread_t schedTid;
    pthread_t sendTid;

    pthread_create(&schedTid, nullptr, schedulerThread, nullptr);
    pthread_create(&sendTid, nullptr, senderThread, nullptr);

    pthread_join(schedTid, nullptr);
    pthread_join(sendTid, nullptr);

    return 0;
}
