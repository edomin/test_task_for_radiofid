#define EPOLL_EVENTS_MAX          4
#define UDP_HELLO_PORT            "4567"
#define UDP_HELLO_RECV_BUFFER_LEN 32

typedef void (*OnEventFunc)(int, void *);
typedef void (*OnTimeoutFunc)(void);

bool WaitInputAndDo(int inputFd, int timeout, OnEventFunc onEventFunc,
 int onEventFd, void *onEventData, OnTimeoutFunc onTimeoutFunc);
