#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAXLEN      256

void *client_callback(void *arg) {

    int client_fd = *(int *)arg;
    char buffer[MAXLEN] = {0};
    
    while (1) {
        int ret = recv(client_fd, buffer, MAXLEN, 0);
        if (ret < 0) {
            close(client_fd);
        } else if (ret == 0) {
            printf("Disconnect\n");
            close(client_fd);
        } else {
            printf("Recv message: %s, %d Bytes\n", buffer, ret);

            send(client_fd, buffer, ret , 0);
        }
    }

}

int main(int argc, char const *argv[])
{
    int sock_fd;
    char buffer[MAXLEN] = {0};

    // 1. 创建套接字
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    // 2. 创建网络结构体
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(struct sockaddr_in));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8888);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // 3. 将套接字和网络结构体绑定
    socklen_t ser_len = sizeof(server_addr);

    if (bind(sock_fd, (struct sockaddr*)&server_addr, ser_len) == -1) {
        printf("bind  socket error: %s(errno: %d)\n", strerror(errno), errno);
        return -1;
    }

    // 4. 将套接字设置成被动
    if (listen(sock_fd, 5) == -1) {
        printf("listen socket error: %s(errno: %d)\n", strerror(errno), errno);
        return -2;
    }

#if 0
    int client_fd;
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(struct sockaddr_in));

    socklen_t client_len = sizeof(struct sockaddr_in);

    char buffer[MAXLEN] = {0};

    client_fd = accept(sock_fd, (struct sockaddr*)&client_addr, &client_len);

    printf("========waiting for client's request========\n");
    while(1) {

        int ret = recv(client_fd, buffer, MAXLEN, 0);
        if (ret < 0) {
            close(client_fd);
        } else if (ret == 0) {
            close(client_fd);
        } else {
            printf("Recv message: %s, %d Bytes\n", buffer, ret);

            send(client_fd, buffer, ret, 0);
        }

    } 
#elif 0

    while(1) {

        int client_fd;
        struct sockaddr_in client_addr;
        memset(&client_addr, 0, sizeof(struct sockaddr_in));
        socklen_t client_len = sizeof(struct sockaddr_in);

        char buffer[MAXLEN] = {0};

        client_fd = accept(sock_fd, (struct sockaddr*)&client_addr, &client_len);

        printf("========waiting for client's request========\n");

        int ret = recv(client_fd, buffer, MAXLEN, 0);
        if (ret < 0) {
            close(client_fd);
        } else if (ret == 0) {
            printf("disconection\n");
            close(client_fd);
        } else {
            printf("Recv message: %s, %d Bytes\n", buffer, ret);

            send(client_fd, buffer, ret, 0);
        }

    }

#elif 0

    while(1) {


        int client_fd;
        struct sockaddr_in client_addr;
        memset(&client_addr, 0, sizeof(struct sockaddr_in));
        socklen_t client_len = sizeof(struct sockaddr_in);

        client_fd = accept(sock_fd, (struct sockaddr*)&client_addr, &client_len);

        pthread_t pthread_id;

        pthread_create(&pthread_id, NULL, client_callback, &client_fd);

    }

#elif 1

    fd_set rfds, rset, wfds, wset;
    int max_fd = sock_fd;
    int i = 0;

    FD_ZERO(&rfds);
    FD_SET(sock_fd, &rfds);

    while(1) {

        rset = rfds;
        wset = wfds;
        
        int nready = select(max_fd + 1, &rset, &wset, NULL, NULL);
        if (nready < 0) {
            printf("select error: %s(errno: %d)\n", strerror(errno), errno);
            continue;
        }

        if (FD_ISSET(sock_fd, &rset)) {
            
            struct sockaddr_in client_addr;
            memset(&client_addr, 0, sizeof(struct sockaddr_in));
            socklen_t client_len = sizeof(struct sockaddr_in);

            int client_fd = accept(sock_fd, (struct sockaddr*)&client_addr, &client_len);
            if (client_fd <= 0) continue;

            char str[INET_ADDRSTRLEN] = {0};
            printf("recvived from %s at port %d, sockfd: %d, clientfd: %d\n", inet_ntop(AF_INET, &client_addr.sin_addr, str, sizeof(str)), ntohs(client_addr.sin_port), sock_fd, client_fd);

            if (max_fd == FD_SETSIZE) {
                printf("clientfd ----> out range\n");
                break;
            }

            FD_SET(client_fd, &rfds);

            if (client_fd > max_fd) max_fd = client_fd;

            printf("sockfd: %d, max_fd: %d, clientfd: %d\n", sock_fd, max_fd, client_fd);

            if (--nready == 0) continue;

        }

        for (i = sock_fd + 1; i <= max_fd; i++) {
            int ret = 0;

            if (FD_ISSET(i, &rset)) {


                ret = recv(i, buffer, MAXLEN, 0);
                if (ret > 0) {
                    printf("recv message: %s, %d Bytes\n", buffer, ret);

                    FD_SET(i, &wfds);
                } else if (ret == 0) {
                    printf("disconnect\n");
                    FD_CLR(i, &rset);
                    close(i);
                }

                if (--nready == 0) break;
            } else if (FD_ISSET(i, &wset)) {
                send(i, buffer, ret, 0);
                FD_SET(i, &rfds);
            }

        }

    }

#endif

    

    return 0;
}