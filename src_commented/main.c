#include <netdb.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "common.h"

/* Все Magic-numbers вынес в константы */

/* Число итераций для первого потока */
#define SYSLOG_PRINT_ITERATIONS_COUNT 10
/* Задержка в микросекундах для первого потока */
#define SYSLOG_PRINT_DELAY_US         320000
/* Адрес сервера */
#define UDP_HELLO_SERVER_ADDRESS      "127.0.0.1"
/* Сообщение, посылаемое на сервер */
#define UDP_HELLO_MESSAGE             "hello"
/* Период ожидания ответа от сервера. Секунды и милисекунды */
#define UDP_HELLO_TIMEOUT_SEC         3
#define UDP_HELLO_TIMEOUT_MSEC        0

/* Функция потока 1 */
void *SyslogPrintInfoIterative(void *unused) {
    /* Открыть соединение с системным логгером */
    openlog ("test_task_for_radiofid", LOG_CONS, LOG_USER);

    /* Цикл из 10-ти итераций */
    for (int iterNum = 0; iterNum < SYSLOG_PRINT_ITERATIONS_COUNT; iterNum++) {
        /* Задержка 320 милисекунд */
        usleep(SYSLOG_PRINT_DELAY_US);
        /* вывод сообщения в syslog */
        syslog(LOG_INFO, "thread-1: step %i\n", iterNum);
    }

    /* Закрыть соединение с системным логгером */
    closelog();

    return NULL;
}

/* Подготовка сокета. Обычно занимает много кода, поэтому выкинута в отдельную
 * функцию */
bool PrepareSocket(int *sockFd, struct addrinfo **servinfo) {
    struct addrinfo hints;            /* Структура с исходными данными
                                       * getaddrinfo */
    int             getaddrInfoError; /* Код возврата функции getaddrinfo */

    /* Обнуление полей структуры hints */
    memset(&hints, 0, sizeof(struct addrinfo));

     /* IPv4. Тут можно указать AF_UNSPEC и getaddrinfo даст варианты с IPv4 и
      * IPv6, но тогда надо чтобы сервер слушал и на IPv4 и на IPv6 (но это не
      * точно) */
    hints.ai_family = AF_INET;
    /* Дейтаграммные сокеты */
    hints.ai_socktype = SOCK_DGRAM;
    /* UDP. Можно не указывать. */
    hints.ai_protocol = IPPROTO_UDP;

    /* Внутри функции getaddrinfo много всякой магии, в том числе преобразование
     * имени хоста в IP-адрес (используя DNS, ./etc/hosts и, возможно, что-то
     * ещё). В servinfo записывается связанный список с подходящими вариантами,
     * исходя из тех, что были записаны в hints. Мы будет использовать только
     * первый элемент списка */
    getaddrInfoError = getaddrinfo(UDP_HELLO_SERVER_ADDRESS, UDP_HELLO_PORT,
     &hints, servinfo);

    if (getaddrInfoError != 0) {
        /* Если произошла другая системная ошибка */
        if (getaddrInfoError == EAI_SYSTEM)
            perror("getaddrinfo");
        /* Если произошла ошибка в функции getaddrinfo */
        else
            fprintf(stderr, "getaddrinfo: %s\n",
             gai_strerror(getaddrInfoError));
        return false;
    }

    /* Создаем сокет. Данные берем из первого элемента servinfo */
    *sockFd = socket((*servinfo)->ai_family, (*servinfo)->ai_socktype,
     (*servinfo)->ai_protocol);
    if (*sockFd == -1) {
        perror("socket");
        freeaddrinfo(*servinfo);
        return false;
    }

    return true;
}

/* Получение ответа от сервера с таймаутом 3 секунды */
bool ReadWithTimeout(int sockFd, struct addrinfo *servinfo) {
    struct timeval timeout;   /* Структура struct timeval, хранищая значение
                               * таймаута. Нужна для функции select */
    fd_set         readFds;
    fd_set         masterFds;

    /* Устанавливаем значение таймаута */
    timeout.tv_sec = UDP_HELLO_TIMEOUT_SEC;
    timeout.tv_usec = UDP_HELLO_TIMEOUT_MSEC;

    /* Обнуляем набор файловых дескрипторов */
    FD_ZERO(&masterFds);
    /* Добавляем сокет в набор файловых дескрипторов */
    FD_SET(sockFd, &masterFds);

    /* Копируем набор файловых дескрипторов в наборор файловых дескрипторов
     * для чтения */
    memcpy(&readFds, &masterFds, sizeof(fd_set));

    /* Ожидаем, когда один или больше файловых дескрипторов с номерами до
     * sockFd + 1 будут готовы для чтения с таймаутом, установленным в
     * переменной timeout */
    if (select(sockFd + 1, &readFds, NULL, NULL, &timeout) < 0) {
        perror("select");
        return false;
    }

    /* Открываем соединение с syslog */
    openlog ("test_task_for_radiofid", LOG_CONS, LOG_USER);

    /* Проверяем, есть ли дескриптор нашего сокета в наборе файловых
     * дескрипторов. Я точно не знаю, как это работает, но предполагаю, что
     * дескриптор сокета должен отвалиться из набора по таймауту */
    if (FD_ISSET(sockFd, &readFds)) {
        char    recvBuffer[UDP_HELLO_RECV_BUFFER_LEN]; /* Буффер для ответа от
                                                        * сервера */
        ssize_t receivedLen;                           /* Число байт, полученных
                                                        * от сервера */

        /* Получаем ответ сервера */
        receivedLen = recvfrom(sockFd, recvBuffer, UDP_HELLO_RECV_BUFFER_LEN, 0,
         servinfo->ai_addr, &servinfo->ai_addrlen);
        recvBuffer[receivedLen] = '\0';

        /* Запись в syslog с ответом */
        syslog(LOG_INFO, "thread-2: recieved: %s\n", recvBuffer);
    } else {
        /* Запись в syslog с ошибкой */
        syslog(LOG_ERR, "%s", "thread-2: recv failed\n");
    }

    /* Закрываем соединение с syslog */
    closelog();

    return true;
}

/* Функция второго потока */
void *UdpHello(void *unused) {
    struct addrinfo *servinfo;     /* Список с данными для общения с сервером от
                                    * функции getaddrinfo. Используем первый
                                    * элемент списка (связанного списка), потому
                                    * что в данном примере это менее геморно */
    int              sockFd;       /* Дескриптор сокета */
    ssize_t          sendToResult; /* КОд возврата функции sendto */

    /* Подготавливаем сокет (магия getaddrinfo(), socket(), обработка ошибок и
     * т. д.) */
    if (!PrepareSocket(&sockFd, &servinfo)) {
        perror("PrepareSocket");
        return NULL;
    }

    /* Отправляем hello на сервер */
    sendToResult = sendto(sockFd, UDP_HELLO_MESSAGE, strlen(UDP_HELLO_MESSAGE),
     0, servinfo->ai_addr, servinfo->ai_addrlen);

    if (sendToResult == -1) {
        perror("sendto");
        return NULL;
    }

    /* Получаем сообщение от сервера с таймаутом 3 секунды */
    ReadWithTimeout(sockFd, servinfo);

    /* Освобождаем список servinfo */
    freeaddrinfo(servinfo);

    return NULL;
}

/* Функция третьего потока. Пустая */
void *Empty(void *unused) {
    return NULL;
}

int main(int argc, char **argv) {
    pthread_t threadSyslogInfoIterative; /* Поток, который пишет в syslog с
                                          * интервалом 320 милисекунд */
    pthread_t threadUdpHello;            /* Поток, который отправляет hello на
                                          * сервер */
    pthread_t threadEmpty;               /* 3-й поток, который ничего не
                                          * делает */

    /* Создаем 3 потока */
    pthread_create(&threadSyslogInfoIterative, NULL, SyslogPrintInfoIterative,
     NULL);
    pthread_create(&threadUdpHello, NULL, UdpHello, NULL);
    pthread_create(&threadEmpty, NULL, Empty, NULL);

    /* Ждём завершения работы 3-х потоков */
    pthread_join(threadSyslogInfoIterative, NULL);
    pthread_join(threadUdpHello, NULL);
    pthread_join(threadEmpty, NULL);

    /* Пишем в stderr строку "app finished" */
    fprintf(stderr, "%s", "app finished\n");

    return 0;
}
