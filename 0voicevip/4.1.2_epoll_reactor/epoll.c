#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <sys/epoll.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

// ./epoll 8080
int main(int argc, char const *argv[])
{
    
    if (argc < 2) {
        printf("The argement is too few\n");
        return -1;
    }

    int port = atoi(argv[1]);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("socket error: %s(errno: %d)\n", strerror(errno), errno);
        return -2;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) < 0) {
        printf("bind error: %s(errno: %d)\n", strerror(errno), errno);
        return -3;
    }

    if (listen(sockfd, 5) < 0) {
        printf("listen error: %s(errno: %d)\n", strerror(errno), errno);
        return -4;
    }

    // epoll opera
    int epfd = epoll_create(1);

    struct epoll_event ev, events[512] = {0};
    ev.events = EPOLLIN;
    ev.data.fd = sockfd;

    // 其中，epfd 是 epoll 实例的文件描述符，sockfd 是要监听的文件描述符，event 是一个结构体，表示要监听的事件类型和对应的文件描述符。
    // 这段代码的含义是，将 sockfd 添加到 epfd 表示的 epoll 实例中，并监听该文件描述符上的可读事件。
    epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);

    while (1) {

        int nready = epoll_wait(epfd, events, 512, -1);
        if (nready < -1) {
            break;
        }

        int i = 0;
        for (i = 0; i < nready; i++) {

            if (events[i].data.fd == sockfd) {

                struct sockaddr_in client_addr;
                memset(&client_addr, 0, sizeof(struct sockaddr_in));
                socklen_t client_len = sizeof(struct sockaddr_in);

                int clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_len);
                if (clientfd < 0) continue;

                char str[INET_ADDRSTRLEN] = {0};
                printf("recv from %s at port %d\n", inet_ntop(AF_INET, &client_addr, str, sizeof(str)),
                    ntohs(client_addr.sin_port));

                ev.events = EPOLLIN | EPOLLET; // | EPOLLET
                ev.data.fd = clientfd;

                epoll_ctl(epfd, EPOLL_CTL_ADD, clientfd, &ev);

            } else {

                int clientfd = events[i].data.fd;

                char buffer[1024] = {0};

                int ret = recv(clientfd, buffer, 5, 0);
                if (ret < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        continue;
                    }

                    close(clientfd);

                    ev.events = EPOLLIN;
                    ev.data.fd = clientfd;
                    epoll_ctl(epfd, EPOLL_CTL_DEL, clientfd, &ev);

                } else if (ret == 0) {

                    printf("disconnect %d\n", clientfd);

                    close(clientfd);

                    ev.events = EPOLLIN;
                    ev.data.fd = clientfd;

                    epoll_ctl(epfd, EPOLL_CTL_DEL, clientfd, &ev);

                } else {
                    printf("Recv message: %s, %d Bytes\n", buffer, ret);
                }


            }

        }


    }

    return 0;
}
