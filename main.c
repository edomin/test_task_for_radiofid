#include <netdb.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "common.h"

#define SYSLOG_PRINT_ITERATIONS_COUNT 10
#define SYSLOG_PRINT_DELAY_US         320000
#define UDP_HELLO_SERVER_ADDRESS      "127.0.0.1"
#define UDP_HELLO_MESSAGE             "hello"
#define UDP_HELLO_TIMEOUT_MS          3000

void *SyslogPrintInfoIterative(void *unused) {
    for (int iterNum = 0; iterNum < SYSLOG_PRINT_ITERATIONS_COUNT; iterNum++) {
        usleep(SYSLOG_PRINT_DELAY_US);
        syslog(LOG_INFO, "thread-1: step %i\n", iterNum);
    }

    return NULL;
}

bool PrepareSocket(int *sockFd, struct addrinfo **servinfo) {
    struct addrinfo hints;
    int             getaddrInfoError;

    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    getaddrInfoError = getaddrinfo(UDP_HELLO_SERVER_ADDRESS, UDP_HELLO_PORT,
     &hints, servinfo);

    if (getaddrInfoError != 0) {
        if (getaddrInfoError == EAI_SYSTEM)
            perror("getaddrinfo");
        else
            fprintf(stderr, "getaddrinfo: %s\n",
             gai_strerror(getaddrInfoError));
        return false;
    }

    *sockFd = socket((*servinfo)->ai_family, (*servinfo)->ai_socktype,
     (*servinfo)->ai_protocol);
    if (*sockFd == -1) {
        perror("socket");
        freeaddrinfo(*servinfo);
        return false;
    }

    return true;
}

void OnReadyToRecvFromServer(int fd, void *servinfo) {
    char             recvBuffer[UDP_HELLO_RECV_BUFFER_LEN];
    ssize_t          receivedLen;
    struct addrinfo *pServinfo = (struct addrinfo *)servinfo;

    receivedLen = recvfrom(fd, recvBuffer, UDP_HELLO_RECV_BUFFER_LEN, 0,
     pServinfo->ai_addr, &pServinfo->ai_addrlen);
    recvBuffer[receivedLen] = '\0';

    syslog(LOG_INFO, "thread-2: recieved: %s\n", recvBuffer);
}

void OnTimeoutToRecvFromServer(void) {
    syslog(LOG_ERR, "%s", "thread-2: recv failed\n");
}

void *UdpHello(void *unused) {
    struct addrinfo *servinfo;
    int              sockFd;
    ssize_t          sendToResult;

    if (!PrepareSocket(&sockFd, &servinfo)) {
        perror("PrepareSocket");
        return NULL;
    }

    sendToResult = sendto(sockFd, UDP_HELLO_MESSAGE, strlen(UDP_HELLO_MESSAGE),
     0, servinfo->ai_addr, servinfo->ai_addrlen);

    if (sendToResult == -1) {
        perror("sendto");
        return NULL;
    }

    WaitInputAndDo(sockFd, UDP_HELLO_TIMEOUT_MS, OnReadyToRecvFromServer,
     sockFd, servinfo, OnTimeoutToRecvFromServer);
    close(sockFd);

    freeaddrinfo(servinfo);

    return NULL;
}

void *Empty(void *unused) {
    return NULL;
}

int main(int argc, char **argv) {
    pthread_t threadSyslogInfoIterative;
    pthread_t threadUdpHello;
    pthread_t threadEmpty;

    openlog("test_task_for_radiofid", LOG_CONS, LOG_USER);

    pthread_create(&threadSyslogInfoIterative, NULL, SyslogPrintInfoIterative,
     NULL);
    pthread_create(&threadUdpHello, NULL, UdpHello, NULL);
    pthread_create(&threadEmpty, NULL, Empty, NULL);

    pthread_join(threadSyslogInfoIterative, NULL);
    pthread_join(threadUdpHello, NULL);
    pthread_join(threadEmpty, NULL);

    closelog();

    fprintf(stderr, "%s", "app finished\n");

    return 0;
}
