#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

#define BUFFER_LENGTH       1024
#define EVENTS_LENGTH       1024

struct sockitem {
    int sockfd;
    int (*callback)(int fd, int event, void *arg);

    char recvBuffer[BUFFER_LENGTH];
    char sendBuffer[BUFFER_LENGTH];

    int rlength;
    int slength;
};

struct reactor {
    int epfd;
    struct epoll_event events[EVENTS_LENGTH];
};

struct reactor *eventloop = NULL;

int recv_cb(int fd, int event, void *arg);

int send_cb(int fd, int event, void *arg) {

    struct sockitem *si = (struct sockitem*)arg;
    struct epoll_event ev;

    int ret = send(fd, si->sendBuffer, si->slength, 0);

    printf("Send message: %s, %d Bytes\n", si->sendBuffer, ret);

    si->sockfd = fd;
    si->callback = recv_cb;

    ev.events = EPOLLIN;
    ev.data.ptr = si;

    epoll_ctl(eventloop->epfd, EPOLL_CTL_MOD, fd, &ev);

    return 0;
}

int recv_cb(int fd, int event, void *arg) {

    struct sockitem *si = (struct sockitem*)arg;
    struct epoll_event ev;

    int ret = recv(fd, si->recvBuffer, BUFFER_LENGTH, 0);
    if (ret < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return -1;
        }

        epoll_ctl(eventloop->epfd, EPOLL_CTL_DEL, fd, &ev);
        close(fd);
        free(si);

        return -2;
    } else if (ret == 0) {
        printf("Disconnect %d\n", fd);

        epoll_ctl(eventloop->epfd, EPOLL_CTL_DEL, fd, &ev);
        close(fd);
        free(si);

        return -3;
    } else {
        printf("Recv message: %s, %d Bytes\n", si->recvBuffer, ret);
        si->rlength = ret;

        memcpy(si->sendBuffer, si->recvBuffer, si->rlength);
        si->slength = si->rlength;

        si->sockfd = fd;
        si->callback = send_cb;

        ev.events = EPOLLOUT;
        ev.data.ptr = si;

        epoll_ctl(eventloop->epfd, EPOLL_CTL_MOD, fd, &ev);

    }

    return 0;
}

int accept_cb(int fd, int event, void *arg) {

    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    socklen_t client_len = sizeof(struct sockaddr_in);

    int clientfd = accept(fd, (struct sockaddr*)&client_addr, &client_len);
    if (clientfd < 0) {
        printf("accept error: %s (errno: %d)\n", strerror(errno), errno);
        return -1;
    }

    char str[INET_ADDRSTRLEN] = {0};
    printf("Recv from %s at port %d\n", inet_ntop(AF_INET, &client_addr, str, sizeof(str)), 
        ntohs(client_addr.sin_port));

    struct sockitem *si = (struct sockitem*)malloc(sizeof(struct sockitem));
    if (si == NULL) {
        printf("malloc error: %s (errno: %d)\n", strerror(errno), errno);
        return -2;
    }
    memset(si, 0, sizeof(struct sockitem));

    si->sockfd = clientfd;
    si->callback = recv_cb;

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.ptr = si;

    epoll_ctl(eventloop->epfd, EPOLL_CTL_ADD, clientfd, &ev);

    return 0;
}

int main(int argc, char const *argv[])
{
    if (argc < 2) {
        printf("The agrement is too few\n");
        return -1;
    }

    int port = atoi(argv[1]);

    // 创建网络套接字
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // 创建信息结构体
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    // 绑定 网络套接字 和 信息结构体
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) == -1) {
        printf("bind error: %s (errno: %d)\n", strerror(errno), errno);
        return -2;
    }

    // 将网络套接字转变为被动
    // listen 函数第二个参数代表 全连接队列的长度为 5 或者 半连接队列 和 全连接队列 之和为 5
    if (listen(sockfd, 5) == -1) {
        printf("listen error: %s (errno: %d)\n", strerror(errno), errno);
        return -3;
    }

    eventloop = (struct reactor*)malloc(sizeof(struct reactor));
    if (eventloop == NULL) {
        printf("malloc error: %s (errno: %d)\n", strerror(errno), errno);
        return -4;
    }
    memset(eventloop, 0, sizeof(struct reactor));

    eventloop->epfd = epoll_create(1);

    struct sockitem *si = (struct sockitem*)malloc(sizeof(struct sockitem));
    if (si == NULL) {
        printf("malloc error: %s (errno: %d)\n", strerror(errno), errno);
        return -4;
    }
    memset(si, 0, sizeof(struct sockitem));

    si->sockfd = sockfd;
    si->callback = accept_cb;

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.ptr = si;

    epoll_ctl(eventloop->epfd, EPOLL_CTL_ADD, sockfd,&ev);

    while (1) {

        int nready = epoll_wait(eventloop->epfd, eventloop->events, EVENTS_LENGTH, -1);
        if (nready == -1) {
            printf("epoll_wait error: %s (errno: %d)\n", strerror(errno), errno);
            return -5;
        }

        int i = 0;
        for (i = 0; i < nready; i++) {

            if (eventloop->events[i].events & EPOLLIN) {
                struct sockitem *si = (struct sockitem*)eventloop->events[i].data.ptr;
                si->callback(si->sockfd, eventloop->events[i].events, si);
            }

            if (eventloop->events[i].events & EPOLLOUT) {
                struct sockitem *si = (struct sockitem*)eventloop->events[i].data.ptr;
                si->callback(si->sockfd, eventloop->events[i].events, si);
            }

        }

    }

    return 0;
}