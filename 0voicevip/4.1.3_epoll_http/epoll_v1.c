#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#define BUFFER_LENGTH   1024
#define EVENTS_LENGTH   1024

struct sockitem {
    int sockfd;
    int (*callback)(int fd, int event, void *arg);

    char rbuffer[BUFFER_LENGTH];
    char wbuffer[BUFFER_LENGTH];

    int rlength;
    int wlength;
};

struct reactor {
    int epfd;
    struct epoll_event events[EVENTS_LENGTH];
};

struct reactor *eventloop = NULL;

int recv_cb(int fd, int event, void *arg);
int send_cb(int fd, int event, void *arg);

int accept_cb(int fd, int event, void *arg) {

    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    socklen_t client_len = sizeof(struct sockaddr_in);

    int clientfd = accept(fd, (struct sockaddr*)&client_addr, &client_len);
    if (clientfd <= 0) {
        printf("accept error: %s (errno: %d)\n", strerror(errno), errno);
        return -1;
    }

    char str[INET_ADDRSTRLEN] = {0};
    printf("Recv from %s at port %d\n", inet_ntop(AF_INET, &client_addr, str, sizeof(str)), 
        ntohs(client_addr.sin_port));

    struct sockitem *si = (struct sockitem*)malloc(sizeof(struct sockitem));
    memset(si, 0, sizeof(struct sockitem));

    si->sockfd = clientfd;
    si->callback = recv_cb;

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.ptr = si;

    epoll_ctl(eventloop->epfd, EPOLL_CTL_ADD, clientfd, &ev);

    return 0;
}

int recv_cb(int fd, int event, void *arg) {

    struct sockitem *si = (struct sockitem*)arg;
    struct epoll_event ev;

    int ret = recv(fd, si->rbuffer, BUFFER_LENGTH, 0);
    if (ret < 0) {

        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // 没有数据可读
            return -1;
        } else {
            // 错误
        }

        epoll_ctl(eventloop->epfd, EPOLL_CTL_MOD, fd, &ev);
        close(fd);
        free(si);

        return -2;
    } else if (ret == 0) {

        printf("Disconnect %d\n", fd);

        epoll_ctl(eventloop->epfd, EPOLL_CTL_MOD, fd, &ev);
        close(fd);
        free(si);
        return -3;
    } else {

        printf("Recv message %s, %d Bytes\n", si->rbuffer, ret);

        si->rlength = ret;

        memcpy(si->wbuffer, si->rbuffer, si->rlength);
        si->wlength = si->rlength;

        si->sockfd = fd;
        si->callback = send_cb;

        ev.events = EPOLLOUT;
        ev.data.ptr = si;

        epoll_ctl(eventloop->epfd, EPOLL_CTL_MOD, fd, &ev);
    }

    return 0;

}

int send_cb(int fd, int event, void *arg) {

    struct sockitem *si = (struct sockitem*)arg;

    send(fd, si->wbuffer, si->wlength, 0);

    struct epoll_event ev;

    si->sockfd = fd;
    si->callback = recv_cb;

    ev.events = EPOLLIN;
    ev.data.ptr = si;

    epoll_ctl(eventloop->epfd, EPOLL_CTL_MOD, fd, &ev);

    return 0;
}

int main(int argc, char const *argv[])
{

    if (argc < 2) {
        printf("The argement is too few \n");
        return -1;
    }

    int port = atoi(argv[1]);

    // 创建套接字
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // 创建网络结构体
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    // 将网络套接字和网络信息结构体绑定
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) < 0) {
        printf("bind error: %s (errno: %d)\n", strerror(errno), errno);
        return -2;
    }

    // 将主动套接字转换为被动套接字
    if (listen(sockfd, 5) < 0) {
        printf("listen error: %s (errno: %d)\n", strerror(errno), errno);
        return -3;
    }

    eventloop = (struct reactor*)malloc(sizeof(struct reactor));
    if (eventloop == NULL) {
        printf("malloc error: %s (errno: %d)\n", strerror(errno), errno);
        return -4;
    }

    memset(eventloop, 0, sizeof(struct reactor));

    // 只有 0 和 1 的区别
    eventloop->epfd = epoll_create(1);

    struct epoll_event ev;
    struct sockitem *si = (struct sockitem*)malloc(sizeof(struct sockitem));

    si->sockfd = sockfd;
    si->callback = accept_cb;

    ev.events = EPOLLIN;
    ev.data.ptr = si;

    epoll_ctl(eventloop->epfd, EPOLL_CTL_ADD, sockfd, &ev);

    while (1) {

        int nready = epoll_wait(eventloop->epfd, eventloop->events, EVENTS_LENGTH, -1);
        if (nready < 0) {
            printf("epoll_wait error: %s (errno: %d)\n", strerror(errno), errno);
            break;
        }

        int i = 0;
        for (i = 0; i < nready; i++) {

            if (eventloop->events[i].events & EPOLLIN) { // read
                struct sockitem *si = (struct sockitem*)eventloop->events[i].data.ptr;
                si->callback(si->sockfd, eventloop->events[i].events, si);
            }

            if (eventloop->events[i].events & EPOLLOUT) { // write
                struct sockitem *si = (struct sockitem*)eventloop->events[i].data.ptr;
                si->callback(si->sockfd, eventloop->events[i].events, si);
            }

        }

    }
    
    return 0;
}