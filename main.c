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
#define UDP_HELLO_TIMEOUT_SEC         3
#define UDP_HELLO_TIMEOUT_MSEC        0

void *SyslogPrintInfoIterative(void *unused) {
    openlog ("test_task_for_radiofid", LOG_CONS, LOG_USER);

    for (int iterNum = 0; iterNum < SYSLOG_PRINT_ITERATIONS_COUNT; iterNum++) {
        usleep(SYSLOG_PRINT_DELAY_US);
        syslog(LOG_INFO, "thread-1: step %i\n", iterNum);
    }

    closelog();

    return NULL;
}

bool PrepareSocket(int *sockFd, struct addrinfo **servinfo) {
    struct addrinfo hints;
    int             getaddrInfoError;

    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_family = AF_UNSPEC;
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

bool ReadWithTimeout(int sockFd, struct addrinfo *servinfo) {
    struct timeval timeout;
    fd_set         readFds;
    fd_set         masterFds;

    timeout.tv_sec = UDP_HELLO_TIMEOUT_SEC;
    timeout.tv_usec = UDP_HELLO_TIMEOUT_MSEC;

    FD_ZERO(&masterFds);
    FD_SET(sockFd, &masterFds);

    memcpy(&readFds, &masterFds, sizeof(fd_set));

    if (select(sockFd + 1, &readFds, NULL, NULL, &timeout) < 0) {
        perror("select");
        return false;
    }

    openlog ("test_task_for_radiofid", LOG_CONS, LOG_USER);

    if (FD_ISSET(sockFd, &readFds)) {
        char    recvBuffer[UDP_HELLO_RECV_BUFFER_LEN];
        ssize_t receivedLen;

        receivedLen = recvfrom(sockFd, recvBuffer, UDP_HELLO_RECV_BUFFER_LEN, 0,
         servinfo->ai_addr, &servinfo->ai_addrlen);
        recvBuffer[receivedLen] = '\0';

        syslog(LOG_INFO, "thread-2: recieved: %s\n", recvBuffer);
    } else {
        syslog(LOG_ERR, "%s", "thread-2: recv failed\n");
    }

    closelog();

    return true;
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

    ReadWithTimeout(sockFd, servinfo);

    freeaddrinfo(servinfo);

    return NULL;
}

int main(int argc, char **argv) {
    // pthread_t thread1;
    // pthread_t thread2;
    // pthread_t thread3;

    // SyslogPrintInfoIterative(NULL);
    UdpHello(NULL);

    fprintf(stderr, "%s", "app finished\n");

    return 0;
}
