#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

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
    memset(&addr, 0, sizeof(struct sockaddr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) == -1) {
        printf("bind error: %s(errno: %d)\n", strerror(errno), errno);
        return -3;
    }

    if (listen(sockfd, 5) == -1) {
        printf("listen error: %s(errno: %d)\n", strerror(errno), errno);
        return -4;
    }

    // epoll opera
    int epfd = epoll_create(1);

    struct epoll_event ev, events[1024] = {0};
    ev.events = EPOLLIN;
    ev.data.fd = sockfd;

    epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);

    while (1) {

        int nready = epoll_wait(epfd, events, 1024, -1);
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
                printf("Recv from %s at port %d\n", inet_ntop(AF_INET, &client_addr, str, sizeof(str)), 
                    ntohs(client_addr.sin_port));

                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = clientfd;

                epoll_ctl(epfd, EPOLL_CTL_ADD, clientfd, &ev);

            } else {

                if (events[i].events & EPOLLIN) { // read

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
                        printf("Disconnect: %d\n", clientfd);

                        close(clientfd);

                        ev.events = EPOLLIN;
                        ev.data.fd = clientfd;

                        epoll_ctl(epfd, EPOLL_CTL_DEL, clientfd, &ev);
                    } else {
                        printf("Recv message: %s, %d Bytes\n", buffer, ret);
                    }

                }

                if (events[i].events & EPOLLOUT) { // write

                    int clientfd = events[i].data.fd;

                    send(clientfd, "hello", 5, 0);

                }

            }

        }

    }

    return 0;
}
