#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#include <openssl/sha.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/evp.h>

#define BUFFER_LENGTH           1024
#define EVENTS_LENGTH           1024
#define GUID 					"258EAFA5-E914-47DA-95CA-C5AB0DC85B11"


// 状态机
enum WEBSOCKET_STATUS {
    WS_HANDSHARK = 0,       // 握手
    WS_TRANSMISSION = 1,    // 通信
    WS_END = 2,             // 终止
};

struct sockitem {
    int sockfd;
    int (*callback)(int fd, int event, void *arg);

    char recvBuffer[BUFFER_LENGTH];
    char sendBuffer[BUFFER_LENGTH];

    int rlength;
    int slength;

    int status;
};

struct reactor {
    int epfd;
    struct epoll_event events[EVENTS_LENGTH];
};

// 大端
struct _nty_ophdr {
    unsigned char   opcode: 4,
                    rsv3: 1,
                    rsv2: 1,
                    rsv1: 1,
                    fin: 1;
    unsigned char   payload_length: 7,
                    mask: 1;
} __attribute__((packed));

struct _nty_websocket_head_126 {
    unsigned short payload_length;
    char mask_key[4];
    unsigned char data[8];
} __attribute__((packed));

struct _nty_websocket_head_127 {
    unsigned long long pyload_length;
    char mask_key[4];
    unsigned char data[8];
} __attribute__((packed));

typedef struct _nty_websocket_head_127 nty_websocket_head_127;
typedef struct _nty_websocket_head_126 nty_websocket_head_126;
typedef struct _nty_ophdr nty_ophdr;

struct reactor *eventloop = NULL;

int recv_cb(int fd, int event, void *arg);
int send_cb(int fd, int event, void *arg);

void umask(char *data, int len, char *mask) {

    int i = 0;

    for (i = 0; i < len; i++) {
        *(data + i) ^= *(mask + (i % 4));
    }

}

char *decode_packet(char *stream, char *mask, int length, int *ret) {

    nty_ophdr *hdr = (nty_ophdr *)stream;
    unsigned char *data = (unsigned char *)stream + sizeof(nty_ophdr);
    int size = 0;
    int start = 0;
    int i = 0;

    if ((hdr->mask & 0x7F) == 126) {

        nty_websocket_head_126 *hdr126 = (nty_websocket_head_126*)data;
        size = hdr126->payload_length;

        for (i = 0; i < 4; i++) {
            mask[i] = hdr126->mask_key[i];
        }

        start = 8;
    } else if ((hdr->mask &0x7F) == 127) {

        nty_websocket_head_127 *hdr127 = (nty_websocket_head_127*)data;
        size = hdr127->pyload_length;

        for (i = 0; i < 4; i++) {
            mask[i] = hdr127->mask_key[i];
        }

        start = 14;
    } else {
        size = hdr->payload_length;

        memcpy(mask, data, 4);
        start = 6;
    }

    *ret = size;
    umask(stream + start, size, mask);

    return stream + start;

}

int encode_packet(char *buffer, char *mask, char *stream, int length) {

    nty_ophdr head = {0};
    head.fin = 1;
    head.opcode = 1;
    int size = 0;

    if (length < 126) {
        head.payload_length = length;
        memcpy(buffer, &head, sizeof(nty_ophdr));
        size = 2;
    } else if(length < 0xffff) {
        nty_websocket_head_126 hdr = {0};
        hdr.payload_length = length;
        memcpy(hdr.mask_key, mask, 4);

        memcpy(buffer, &head, sizeof(nty_ophdr));
        memcpy(buffer + sizeof(nty_ophdr), &hdr, sizeof(nty_websocket_head_126));
        size = sizeof(nty_websocket_head_126);
    } else {

        nty_websocket_head_127 hdr = {0};
        hdr.pyload_length = length;
        memcpy(hdr.mask_key, mask, 4);

        memcpy(buffer, &head, sizeof(nty_ophdr));
        memcpy(buffer + sizeof(nty_ophdr), &hdr, sizeof(nty_websocket_head_127));

        size = sizeof(nty_websocket_head_127);
    }

    memcpy(buffer + 2, stream, length);

    return length + 2;
}

int base64_encode(char *in_str, int in_len, char *out_str) {

    BIO *b64, *bio;
    BUF_MEM *bptr = NULL;
    size_t size = 0;

    if (in_str == NULL || out_str == NULL) {
        return -1;
    }

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    BIO_write(bio, in_str, in_len);
    BIO_flush(bio);

    BIO_get_mem_ptr(bio, &bptr);
    memcpy(out_str, bptr->data, bptr->length);
    out_str[bptr->length - 1] = '\0';
    size = bptr->length;

    BIO_free_all(bio);

    return size;
}

int readline(char *allbuf, int level, char *linebuf) {

    int len = strlen(allbuf);

    for (; level < len; level++) {

        if (allbuf[level] == '\r' && allbuf[level + 1] == '\n') {
            return level + 2;
        } else {
            *(linebuf++) = allbuf[level];
        }

    }

    return -1;
}

int handshark(struct sockitem *si, struct reactor *mainloop) {

    char linebuf[256];
    char sec_accept[32];
    int level = 0;
    unsigned char sha1_data[SHA_DIGEST_LENGTH + 1] = {0};
    char head[BUFFER_LENGTH] = {0};

    do {

        memset(linebuf, 0, sizeof(linebuf));
        level = readline(si->recvBuffer, level, linebuf);

        // 找到Sec-WebSocket-Key这一行
        if (strstr(linebuf, "Sec-WebSocket-Key") != NULL) {

            // 1. key=KEY+GUID
            strcat(linebuf, GUID);

            // 2. sha_key = SHA1(key)
            SHA1((unsigned char*)&linebuf + 19, strlen(linebuf + 19), (unsigned char *)&sha1_data);

            // 3. sec_key=base64_encode(sha_key)
            base64_encode((char *)sha1_data, strlen((char *)sha1_data), sec_accept);
            sprintf(head,   "HTTP/1.1 101 Switching Protocols\r\n" \
                            "Upgrade: websocket\r\n" \
                            "Connection: Upgrade\r\n" \
                            "Sec-WebSocket-Accept: %s\r\n" \
                            "\r\n", sec_accept);

            printf("[handshark response]\n");
            printf("%s\n\n\n", head);

            memset(si->recvBuffer, 0, BUFFER_LENGTH);
            memcpy(si->sendBuffer, head, strlen(head));
            si->slength = strlen(head);

            struct epoll_event ev;

            ev.events = EPOLLOUT | EPOLLET;
            si->sockfd = si->sockfd;
            si->callback = send_cb;
            si->status = WS_TRANSMISSION;
            ev.data.ptr = si;

            epoll_ctl(mainloop->epfd, EPOLL_CTL_MOD, si->sockfd, &ev);

            break;
        }
        

    } while((si->recvBuffer[level] != '\r' || si->recvBuffer[level + 1] != '\n') && level != -1);

    return 0;
}

int transform(struct sockitem *si, struct reactor *mainloop) {

    int ret = 0;
    char mask[4] = {0};
    char *data = decode_packet(si->recvBuffer, mask, si->rlength, &ret);

    printf("data: %s, length: %d\n", data, ret);

    ret = encode_packet(si->sendBuffer, mask, data, ret);
    si->slength = ret;

    struct epoll_event ev;
    ev.events = EPOLLOUT | EPOLLET;
    si->sockfd = si->sockfd;
    si->callback = send_cb;
    si->status = WS_TRANSMISSION;
    ev.data.ptr = si;

    epoll_ctl(mainloop->epfd, EPOLL_CTL_MOD, si->sockfd, &ev);

    return 0;
}

int send_cb(int fd, int event, void *arg) {

    struct sockitem *si = (struct sockitem*)arg;
    struct epoll_event ev;

    int ret = send(fd, si->sendBuffer, si->slength, 0);

    // printf("Send message: %s, %d Bytes\n", si->sendBuffer, ret);

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

        ev.events = EPOLLIN;

        epoll_ctl(eventloop->epfd, EPOLL_CTL_DEL, fd, &ev);
        close(fd);
        free(si);

    } else if (ret == 0) {
        printf("Disconnect %d\n", fd);

        ev.events = EPOLLIN;

        epoll_ctl(eventloop->epfd, EPOLL_CTL_DEL, fd, &ev);
        close(fd);
        free(si);
    } else {

        si->rlength = 0;

        if (si->status == WS_HANDSHARK) {

            printf("[recv_cb request]\n");
            printf("%s\n", si->recvBuffer);

            handshark(si, eventloop);

        } else if (si->status == WS_TRANSMISSION) {
			transform(si, eventloop);
        } else if (si->status == WS_END) {

        }

    }

    return 0;
}

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
    if (NULL == si) {
        printf("malloc error: %s (errno: %d)\n", strerror(errno), errno);
        return -2;
    }

    memset(si, 0, sizeof(struct sockitem));

    si->sockfd = clientfd;
    si->callback = recv_cb;
    si->status = WS_HANDSHARK;

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

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) == -1) {
        printf("bind error: %s (errno: %d)\n", strerror(errno), errno);
        return -2;
    }

    if (listen(sockfd, 5) == -1) {
        printf("listen error: %s (errno: %d)\n", strerror(errno), errno);
        return -3;
    }
    
    eventloop = (struct reactor*)malloc(sizeof(struct reactor));
    memset(eventloop, 0, sizeof(struct reactor));

    eventloop->epfd = epoll_create(1);

    struct sockitem *si = (struct sockitem*)malloc(sizeof(struct sockitem));
    memset(si, 0, sizeof(struct sockitem));

    si->sockfd = sockfd;
    si->callback = accept_cb;

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.ptr = si;

    epoll_ctl(eventloop->epfd, EPOLL_CTL_ADD, sockfd, &ev);

    while (1) {

        int nready = epoll_wait(eventloop->epfd, eventloop->events, EVENTS_LENGTH, -1);
        if (nready <= 0) {
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