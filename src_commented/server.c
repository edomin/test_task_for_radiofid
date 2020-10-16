#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "common.h"

/*
 * Мне лень тут всё подробно комментировать. Здесь всё похоже на код второго
 * потока основной программы. Сервер имеет свою функцию PrepareSocket, потому
 * что она тут немного отличается от той, что в основной программе. Например,
 * здесь есть bind(), а ещё здесь явно не указывается IP-адрес сервера)
 */

#define UDP_HELLO_MESSAGE "bye"

bool PrepareSocket(int *sockFd, struct addrinfo **servinfo) {
    struct addrinfo hints;
    int             getaddrInfoError;

    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    hints.ai_flags = AI_PASSIVE;

    getaddrInfoError = getaddrinfo(NULL, UDP_HELLO_PORT,
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

    if (bind(*sockFd, (*servinfo)->ai_addr, (*servinfo)->ai_addrlen) < 0) {
        perror("bind");
        close(*sockFd);
        freeaddrinfo(*servinfo);
        return false;
    }

    freeaddrinfo(*servinfo);

    return true;
}

int main() {
    struct addrinfo        *servinfo;
    int                     sockFd;
    struct sockaddr_storage cliAddr;
    socklen_t               cliAddrLen;
    char                    recvBuffer[UDP_HELLO_RECV_BUFFER_LEN];
    ssize_t                 receivedLen;
    ssize_t                 sendToResult;

    if (!PrepareSocket(&sockFd, &servinfo)) {
        perror("PrepareSocket");
        return 1;
    }

    cliAddrLen = sizeof(struct sockaddr_storage);
    receivedLen = recvfrom(sockFd, recvBuffer, UDP_HELLO_RECV_BUFFER_LEN , 0,
     (struct sockaddr *)&cliAddr, &cliAddrLen);

    if (receivedLen == -1) {
        perror("recvfrom");
        return 2;
    }

    sendToResult = sendto(sockFd, UDP_HELLO_MESSAGE, strlen(UDP_HELLO_MESSAGE),
     0, (struct sockaddr *)&cliAddr, cliAddrLen);

    if (sendToResult == -1) {
        perror("sendto");
        return 3;
    }

    return 0;
}
