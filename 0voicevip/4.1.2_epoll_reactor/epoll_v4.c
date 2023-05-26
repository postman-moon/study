#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

#define EVENTS_LENGTH   1024

struct sockitem {
    int sockfd;
    int (*callback)(int fd, int event, void *arg);
};

struct reactor {
    int epfd;
    struct epoll_event events[EVENTS_LENGTH];
};

struct reactor *eventloop = NULL;

int recv_callback(int fd, int event, void *arg);
int send_callback(int fd, int event, void *arg);

int accept_callback(int fd, int event, void *arg) {

    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    socklen_t client_len = sizeof(struct sockaddr_in);

    int clientfd = accept(fd, (struct sockaddr*)&client_addr, &client_len);
    if (clientfd < 0) {
        printf("accept error: %s(errno: %d)\n", strerror(errno), errno);
        return -1;
    }

    char str[INET_ADDRSTRLEN] = {0};
    printf("Recv from %s at port %d\n", inet_ntop(AF_INET, &client_addr, str, sizeof(str)), 
        ntohs(client_addr.sin_port));

    struct sockitem *si = (struct sockitem*)arg;

    si->sockfd = clientfd;
    si->callback = recv_callback;

    struct epoll_event ev;

    ev.events = EPOLLIN;
    ev.data.ptr = si;

    epoll_ctl(eventloop->epfd, EPOLL_CTL_ADD, clientfd, &ev);

    return 0;
}

int recv_callback(int fd, int event, void *arg) {

    char buffer[1024] = {0};
    struct sockitem *si = (struct sockitem*)arg;
    struct epoll_event ev;

    int ret = recv(fd, buffer, 1024, 0);
    if (ret < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return -1;
        }

        epoll_ctl(eventloop->epfd, EPOLL_CTL_DEL, fd, &ev);
        close(fd);
        free(si);
        return -2;

    } else if (ret == 0) {
        printf("Disconnect %d", fd);

        epoll_ctl(eventloop->epfd, EPOLL_CTL_DEL, fd, &ev);
        close(fd);
        free(si);

        return -3;
    } else {
        printf("Recv message: %s, %d Bytes\n", buffer, ret);

        si->sockfd = fd;
        si->callback = send_callback;
        ev.events = EPOLLOUT | EPOLLET;
        ev.data.ptr = si;

        epoll_ctl(eventloop->epfd, EPOLL_CTL_MOD, fd, &ev);

        return 0;
    }

}

int send_callback(int fd, int event, void *arg) {

    struct sockitem *si = (struct sockitem*)malloc(sizeof(struct sockitem));
    struct epoll_event ev;

    send(fd, "hello", 5, 0);

    si->sockfd = fd;
    si->callback = recv_callback;

    ev.events = EPOLLIN;
    ev.data.ptr = si;

    epoll_ctl(eventloop->epfd, EPOLL_CTL_MOD, fd, &ev);

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

    // 创建网络结构体
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    
    // 将网络套接字和网络结构体绑定在一起
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) == -1) {
        printf("bind error: %s(errno: %d)\n", strerror(errno), errno);
        return -2;
    }

    // 将网络套接字转变为被动
    if (listen(sockfd, 5) == -1) {
        printf("listen error: %s(errno: %d)\n", strerror(errno), errno);
        return -3;
    }

    eventloop = (struct reactor*)malloc(sizeof(struct reactor));

    eventloop->epfd = epoll_create(1);

    struct sockitem *si = (struct sockitem*)malloc(sizeof(struct sockitem));
    memset(si, 0, sizeof(struct sockitem));

    si->sockfd = sockfd;
    si->callback = accept_callback;

    struct epoll_event ev;
    memset(&ev, 0, sizeof(struct epoll_event));

    ev.events = EPOLLIN;
    ev.data.ptr = si;

    epoll_ctl(eventloop->epfd, EPOLL_CTL_ADD, sockfd, &ev);

    while (1) {

        int nready = epoll_wait(eventloop->epfd, eventloop->events, EVENTS_LENGTH, -1);
        if (nready < -1) {
            printf("epoll_wait error: %s(errno: %d)\n", strerror(errno), errno);
            break;
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
