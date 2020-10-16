#include <netdb.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#define SYSLOG_PRINT_ITERATIONS_COUNT 10
#define SYSLOG_PRINT_DELAY_US         320000
#define UDP_HELLO_SERVER_ADDRESS      "127.0.0.1"
#define UDP_HELLO_PORT                "4567"
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

void *UdpHello(void *unused) {
    struct addrinfo *servinfo;
    int              sockFd;
    ssize_t          sendToResult;
    // struct timeval   timeout;
    // fd_set readfds, masterfds;

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

    // timeout.tv_sec = 3;
    // timeout.tv_usec = 0;

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
