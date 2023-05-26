#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

struct sockitem {

    int sockfd;
    int (*callback)(int fd, int event, void *arg);

};

int epfd;

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

    struct epoll_event ev;

    struct sockitem *si = (struct sockitem*)malloc(sizeof(struct sockitem));
    if (si == NULL) {
        printf("malloc error: %s(errno: %d)\n", strerror(errno), errno);
        return -2;
    }
    memset(si, 0, sizeof(struct sockitem));

    si->callback = recv_callback;
    si->sockfd = clientfd;
    ev.events = EPOLLIN;
    ev.data.ptr = si;

    epoll_ctl(epfd, EPOLL_CTL_ADD, clientfd, &ev);

    return 0;
}

int recv_callback(int fd, int event, void *arg) {

    char buffer[1024] = {0};
    struct sockitem *si = (struct sockitem*)malloc(sizeof(struct sockitem));
    struct epoll_event ev;

    int ret = recv(fd, buffer, 1024, 0);
    if (ret < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return -1;
        }

        ret = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &ev);
        close(fd);
        free(si);
        printf("epoll_ctl_del ret = %d", ret);
        return -2;
    } else if (ret == 0) {
        printf("Disconnect %d", ret);

        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &ev);
        close(fd);
        free(si);
        return -3;
    } else {
        printf("Recv message: %s, %d Bytes\n", buffer, ret);

        si->sockfd = fd;
        si->callback = send_callback;
        ev.events = EPOLLIN | EPOLLET;
        ev.data.ptr = si;

        epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
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

    epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);

    return 0;
}

int main(int argc, char const *argv[])
{
    
    if (argc < 2) {
        printf("The argement is too few\n");
        return -1;
    }

    int port = atoi(argv[1]);

    // 创建套接字
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket error: %s(errno: %d)\n", strerror(errno), errno);
        return -2;
    }

    // 创建网络信息结构体
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));

    // 使用IPV4
    addr.sin_family = AF_INET;
    // 绑定端口号
    addr.sin_port = htons(port);
    // 绑定本机的所有网卡
    addr.sin_addr.s_addr = INADDR_ANY;

    // 将网络套接字和网络信息结构体进行绑定
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) == -1) {
        printf("bind error: %s(errno: %d)\n", strerror(errno), errno);
        return -3;
    }

    // 全连接队列为 5 或者 全连接和半连接和为 5  和版本有关
    if (listen(sockfd, 5) == -1) {
        printf("listen error: %s(errno: %d)\n", strerror(errno), errno);
        return -4;
    }

    // epoll opera
    // 只有 0 和 1 的区别，和之前 epoll 实现有关系
    epfd = epoll_create(1);

    struct epoll_event ev, events[1024] = {0};
    struct sockitem *it = (struct sockitem*)malloc(sizeof(struct sockitem));
    memset(it, 0, sizeof(struct sockitem));

    ev.events = EPOLLIN;
    it->sockfd = sockfd;
    it->callback = accept_callback;
    ev.data.ptr = it;

    epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);

    while (1) {

        int nready = epoll_wait(epfd, events, 1024, -1);
        if (nready < -1) {
            break;
        }

        int i = 0;
        for (i = 0; i < nready; i++) {

            if (events[i].events & EPOLLIN) {
                struct sockitem *si = (struct sockitem*)events[i].data.ptr;
                si->callback(si->sockfd, EPOLLIN, si);
            }

            if (events[i].events & EPOLLOUT) {
                struct sockitem *si = (struct sockitem*)events[i].data.ptr;
                si->callback(si->sockfd, EPOLLOUT, si);
            }

        }

    }

    return 0;
}
