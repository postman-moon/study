#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

int sockfd;

void do_sigio(int signo) {

    char buffer[256] = {0};

    struct sockaddr_in clie_addr;
    memset(&clie_addr, 0, sizeof(struct sockaddr_in));

    socklen_t clie_len = sizeof(struct sockaddr_in);

    int ret = recvfrom(sockfd, buffer, 256, 0, (struct sockaddr*)&clie_addr, &clie_len);
    printf("Recv Message: %s, %d Bytes\n", buffer, ret);

    sendto(sockfd, buffer, ret, 0, (struct sockaddr*)&clie_addr, clie_len);

}

int main() {

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    signal(SIGIO, do_sigio);

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(struct sockaddr_in));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8888);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    socklen_t addr_len = sizeof(serv_addr);

    // 将sockfd的拥有者设置为当前进程
    fcntl(sockfd, F_SETOWN, getpid());

    // 获取套接字文件描述符sockfd的当前状态标志位
    // O_NONBLOCK（非阻塞模式）、O_ASYNC（异步通知模式）
    int flags = fcntl(sockfd, F_GETFL, 0);
    flags |= O_ASYNC | O_NONBLOCK;

    // 用于设置文件描述符的属性
    fcntl(sockfd, F_SETFL, flags);

    bind(sockfd, (struct sockaddr*)&serv_addr, addr_len);

    while(1) sleep(1);

    return 0;
}