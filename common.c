#include "common.h"

#include <stdbool.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <unistd.h>

bool WaitInputAndDo(int inputFd, int timeout, OnEventFunc onEventFunc,
 int onEventFd, void *onEventData, OnTimeoutFunc onTimeoutFunc) {
    struct epoll_event event;
    struct epoll_event events[EPOLL_EVENTS_MAX];
    int                eventsCount;
    int                epollFd = epoll_create1(0);

    if (epollFd == -1) {
        perror("epoll_create1");
        return false;
    }

    event.events = EPOLLIN | EPOLLET;
    event.data.fd = inputFd;

    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, inputFd, &event) == -1) {
        perror("epoll_ctl");
        close(epollFd);
        return false;
    }

    eventsCount = epoll_wait(epollFd, events, EPOLL_EVENTS_MAX, timeout);

    if (eventsCount == -1) {
        perror("epoll_wait");
        close(epollFd);
        return false;
    }

    if ((eventsCount == 0) && (onTimeoutFunc != NULL))
        onTimeoutFunc();

    for (int i = 0; i < eventsCount; i++)
        onEventFunc(onEventFd, onEventData);

    close(epollFd);

    return true;
}
