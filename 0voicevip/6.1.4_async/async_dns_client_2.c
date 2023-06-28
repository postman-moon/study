#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <sys/epoll.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <pthread.h>

#define DNS_SVR				"114.114.114.114"

#define DNS_HOST			0x01
#define DNS_CNAME			0x05

#define ASYNC_CLIENT_NUM		1024

// DNS 报文头部
struct dns_header {
	unsigned short id;				// 会话标识
	unsigned short flags;			// 标志位

	unsigned short qdcount;			// 问题数
	unsigned short ancount;			// 回答，资源记录数
	
	unsigned short nscount;			// 授权，资源记录数
	unsigned short arcount;			// 附加，资源记录数
};

// DNS报文正文
struct dns_question {
	int length;
	// 占2个字节。表示RR类型
	unsigned short qtype;
	// 占2个字节。表示RR分类
	unsigned short qclass;
	char *qname;
};

// DNS服务器返回的ip信息
struct dns_item {
	char *domain;
	char *ip;
};


typedef void (*async_result_cb)(struct dns_item *list, int count);

// 异步请求池上下文
struct async_context {
	int epfd;				// epoll 句柄
	pthread_t threadid;		// 线程 id
};

// epoll 使用的参数
struct ep_arg {
	int sockfd;				// socketfd
	async_result_cb cb;		// 处理返回结果用的回调函数
};

// 创建 DNS 协议中的 header
int dns_create_header(struct dns_header *header) {

	if (header == NULL) {
		return -1;
	}

	// 置空 header
	memset(header, 0, sizeof(struct dns_header));

	// 给 header 的 id 随机赋值
	srandom(time(NULL));
	header->id = random();

	// 统一转化为网络字节序
	header->flags |= htons(0x0100);
	header->qdcount = htons(1);

	return 0;
}

// 创建 DNS 协议中的 question
int dns_create_question(struct dns_question *question, const char *hostname) {

	if (question == NULL || hostname == NULL) {
		return -1;
	}

	memset(question, 0, sizeof(struct dns_question));

	// name 的长度是不固定的，c 字符串还要占用一个 '\0'，多开两个空间备用
	question->qname = (char *)malloc(strlen(hostname) + 2);
	if (question->qname == NULL) {
		perror("malloc");
		return -2;
	}

	// 设置 question 的数据长度信息
	question->length = strlen(hostname) + 2;
	// 设置 question 的类型 A类
	question->qtype = htons(1);
	// 设置 question 的 class，通常为 1
	question->qclass = htons(1);

	// 填充 name 
	const char delim[2] = ".";
	char *qname_p = question->qname;

	// strdup是深拷贝，里面会调用malloc函数，最后要记得释放
	char *hostname_dup = strdup(hostname);
	// 分解字符串 hostname_dup 为一组字符串，delim 为分隔符
	char *token = strtok(hostname_dup, delim);

	// 继续分割
	while (token != NULL) {
		
		// 得到上一次截取的长度
		size_t len = strlen(token);

		// 把长度加到字符串中
		*qname_p = len;
		// 后移指针
		qname_p ++;

		// 把 token 放到 qname_p 里面，复制长度是 len + 1，是把结尾的/0也放进去
		strncpy(qname_p, token, len + 1);
		qname_p += len;

		// 从上次结束的地方开始截取
		token = strtok(NULL, delim);

	}

	free(hostname_dup);

	return 0;
}

// 对头部和问题区做一个打包
int dns_build_request(struct dns_header *header, struct dns_question *question, char *request) {

	if (header == NULL || question == NULL || request == NULL) {
		return -1;
	}

	int header_s = sizeof(struct dns_header);
	int question_s = question->length + sizeof(question->qtype) + sizeof(question->qclass);

	int length = question_s +  header_s;

	int offset = 0;
	memcpy(request+offset, header, sizeof(struct dns_header));
	offset += sizeof(struct dns_header);

	memcpy(request + offset, question->qname, question->length);
	offset += question->length;

	memcpy(request + offset, &question->qtype, sizeof(question->qtype));
	offset += sizeof(question->qtype);

	memcpy(request + offset, &question->qclass, sizeof(question->qclass));

	return length;
}

// 解析服务器发过来的数据
static int is_pointer(int in) {

	return ((in & 0xC0) == 0xC0);

}

// 开启非阻塞
// fcntl 系统调用可以用来对已打开的文件描述符进行各种控制操作以改变已打开文件的的各种属性
static int set_block(int fd, int block) {

	// 查询 fd 的状态
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0) {
		return flags;
	}

	// 开启非阻塞
	if (block) {
		flags &= ~O_NONBLOCK;
	} else {
		flags |= O_NONBLOCK;
	}

	if (fcntl(fd, F_SETFL, flags) < 0) {
		return -1;
	}

	return 0;
}


// DNS 解析名称
static void dns_parse_name(unsigned char *chunk, unsigned char *ptr, char *out, int *len) {

	int flag = 0, n = 0;
	char *pos = out + (*len);

	while (1) {

		flag = (int)ptr[0];
		if (flag == 0) break;

		if (is_pointer(flag)) {

			n = (int)ptr[1];
			ptr = chunk + n;
			dns_parse_name(chunk, ptr, out, len);
			break;

		} else {

			ptr ++;
			memcpy(pos, ptr, flag);
			pos += flag;
			ptr += flag;

			*len += flag;

			if ((int)ptr[0] != 0) {
				memcpy(pos, ".", 1);
				pos += 1;
				(*len) += 1;
			}

		}

	}

}

// 解析响应信息
static int dns_parse_response(char *buffer, struct dns_item **domains) {

	int i = 0;
	unsigned char *ptr = (unsigned char *)buffer;

	ptr += 4;
	int querys = ntohs(*(unsigned short *)ptr);

	ptr += 2;
	int answers = ntohs(*(unsigned short *)ptr);

	ptr += 6;
	for (i = 0; i < querys; i++) {
		while (1) {
			int flag = (int)ptr[0];
			ptr += (flag + 1);

			if (flag == 0) break;
		}
		ptr += 4;
	}

	char cname[128], aname[128], ip[20], netip[4];
	int len, type, ttl, datalen;

	int cnt = 0;
	struct dns_item *list = (struct dns_item *)calloc(answers, sizeof(struct dns_item));
	if (list == NULL) {
		return -1;
	}

	printf("answers: %d\n", answers);

	for (i = 0; i < answers; i++) {

		memset(aname, 0, sizeof(aname));
		len = 0;

		dns_parse_name((unsigned char *)buffer, ptr, aname, &len);
		ptr += 2;

		type = htons(*(unsigned short *)ptr);
		ptr += 4;

		ttl = htons(*(unsigned short *)ptr);
		ptr += 4;

		datalen = ntohs(*(unsigned short *)ptr);
		ptr += 2;

		if (type == DNS_CNAME) {

			memset(cname, 0, sizeof(cname));
			len = 0;
			dns_parse_name((unsigned char *)buffer, ptr, cname, &len);
			ptr += datalen;

		} else if (type == DNS_HOST) {

			memset(ip, 0, sizeof(ip));

			if (datalen == 4) {

				memcpy(netip, ptr, datalen);
				inet_ntop(AF_INET, netip, ip, sizeof(struct sockaddr));

				printf("%s has address %s \n", aname, ip);
				printf("\tTime to live: %d minutes, %d seconds\n", ttl / 60, ttl % 60);

				list[cnt].domain = (char *)calloc(strlen(aname) + 1, 1);
				memcpy(list[cnt].domain, aname, strlen(aname));

				list[cnt].ip = (char *)calloc(strlen(ip) + 1, 1);
				memcpy(list[cnt].ip, ip, strlen(ip));

				cnt ++;
			}

			ptr += datalen;
		}

	}

	*domains = list;
	ptr += 2;

	return cnt;
}

int dns_client_commit(const char *domain) {

	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		perror("create socket failed\n");
		exit(-1);
	}

	printf("url:%s\n", domain);

	set_block(sockfd, 0); //nonblock

	struct sockaddr_in dest;
	bzero(&dest, sizeof(dest));
	dest.sin_family = AF_INET;
	dest.sin_port = htons(53);
	dest.sin_addr.s_addr = inet_addr(DNS_SVR);
	
	int ret = connect(sockfd, (struct sockaddr*)&dest, sizeof(dest));
	printf("connect :%d\n", ret);

	struct dns_header header = {0};
	dns_create_header(&header);

	struct dns_question question = {0};
	dns_create_question(&question, domain);

	char request[1024] = {0};
	int req_len = dns_build_request(&header, &question, request);
	int slen = sendto(sockfd, request, req_len, 0, (struct sockaddr*)&dest, sizeof(struct sockaddr));

	while (1) {
		char buffer[1024] = {0};
		struct sockaddr_in addr;
		size_t addr_len = sizeof(struct sockaddr_in);
	
		int n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&addr, (socklen_t*)&addr_len);
		// printf("n: %d\n", n);
		if (n <= 0) continue;
		
		printf("recvfrom n : %d\n", n);
		struct dns_item *domains = NULL;
		dns_parse_response(buffer, &domains);

		break;
	}

	return 0;
}

// 销毁资源
void dns_async_client_free_domains(struct dns_item *list, int count) {

	int i = 0;

	for (i = 0; i < count; i++) {
		free(list[i].domain);
		free(list[i].ip);
	}

	free(list);

}

/* 
epoll 线程的回调函数 callback
while(1){
    epoll_wait();
    recv;
    parser();
    data callback();
    epoll_ctl(ep_fd, EPOLL_CTL_DEL, sockfd, NULL);
    free(date);
 }
 */
static void *dns_async_callback(void *arg) {

	struct async_context *ctx = (struct async_context*)arg;

	int epfd = ctx->epfd;

	while (1) {

		struct epoll_event events[ASYNC_CLIENT_NUM] = {0};

		int nready = epoll_wait(epfd, events, ASYNC_CLIENT_NUM, -1);

		if (nready < 0) {
			if (errno == EINTR || errno == EAGAIN) {
				continue;
			} else {
				break;
			}
		} else if (nready == 0) {
			continue;
		}

		printf("nready: %d\n", nready);

		int i = 0;
		for (i = 0; i < nready; i ++) {

			struct ep_arg *ptr = (struct ep_arg *)events[i].data.ptr;
			int sockfd = ptr->sockfd;

			char buffer[1024] = {0};
			struct sockaddr_in addr;
			size_t addr_len = sizeof(struct sockaddr_in);

			// recv
			recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&addr, (socklen_t*)&addr_len);

			// parser();
			struct dns_item *domain_list = NULL;
			int count = dns_parse_response(buffer, &domain_list);

			// data callback();
			ptr->cb(domain_list, count);

			// epoll_ctl(ep_fd, EPOLL_CTL_DEL, sockfd, NULL);
			epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, NULL);
			close(sockfd);

			// free
			dns_async_client_free_domains(domain_list, count);
			free(ptr);


		}

	}

	return NULL;
}

/* 
初始化请求池 init
1. malloc ctx
2. epoll_create
3. pthread_create
 */
struct async_context *dns_async_client_init() {

	int epfd = epoll_create(1);
	if (epfd < 0) {
		perror("epoll_create");
		return NULL;
	}

	struct async_context *ctx = calloc(1, sizeof(struct async_context));
	if (ctx == NULL) {
		perror("calloc");
		close(epfd);
		return NULL;
	}
	ctx->epfd = epfd;

	int ret = pthread_create(&ctx->threadid, NULL, dns_async_callback, ctx);
	if (ret) {
		perror("pthread_create");
		close(epfd);
		free(ctx);
		return NULL;
	}
	usleep(1); //child go first

	return ctx;
}

/* 
销毁请求池 destroy
1. close (epfd);
2. pthread_cancel()
3. free
 */
int dns_async_client_destroy(struct async_context *ctx) {

	close(ctx->epfd);
	pthread_cancel(ctx->threadid);
	free(ctx);

	return 0;

}

/* 
建立连接提交请求 commit
1.socket 创建socket 
2.connect连接到第三方服务 
3.encode--->mysql/redis/dns 根据对应的协议将发送的数据封装好 
4.send将数据发送出去 
5.epoll_ctl(ctx->epfd, EPOLL_CTL_ADD, sockfd, &ev);把fd加入到epoll中
 */
int dns_async_client_commit(struct async_context* ctx, const char *domain, async_result_cb cb) {

	// 1. 创建 socket
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		perror("socket");
		return -1;
	}

	printf("url: %s\n", domain);

	// nonoblock
	set_block(sockfd, 0);

	// 2.connect连接到第三方服务 
	struct sockaddr_in dest;
	memset(&dest, 0, sizeof(struct sockaddr_in));

	dest.sin_family = AF_INET;
	dest.sin_port = htons(53);
	dest.sin_addr.s_addr = inet_addr(DNS_SVR);

	int ret = connect(sockfd, (struct sockaddr*)&dest, sizeof(struct sockaddr_in));
	if (ret < 0) {
		perror("connect");
		return -2;
	}

	// 3.encode--->mysql/redis/dns 根据对应的协议将发送的数据封装好 
	struct dns_header header = {0};
	dns_create_header(&header);

	struct dns_question question = {0};
	dns_create_question(&question, domain);

	char request[1024] = {0};
	int req_len = dns_build_request(&header, &question, request);

	// 4.send将数据发送出去 
	sendto(sockfd, request, req_len, 0, (struct sockaddr*)&dest, sizeof(struct sockaddr));

	// 5.epoll_ctl(ctx->epfd, EPOLL_CTL_ADD, sockfd, &ev);把fd加入到epoll中
	struct ep_arg *ptr = (struct ep_arg*)calloc(1, sizeof(struct ep_arg));
	if (ptr == NULL) {
		perror("calloc");
		return -3;
	}
	ptr->sockfd = sockfd;
	ptr->cb = cb;

	struct epoll_event ev;
	ev.data.ptr = ptr;
	ev.events = EPOLLIN;

	ret = epoll_ctl(ctx->epfd, EPOLL_CTL_ADD, sockfd, &ev);

	return ret;

} 


char *domain[] = {
	"www.ntytcp.com",
	"bojing.wang",
	"www.baidu.com",
	"tieba.baidu.com",
	"news.baidu.com",
	"zhidao.baidu.com",
	"music.baidu.com",
	"image.baidu.com",
	"v.baidu.com",
	"map.baidu.com",
	"baijiahao.baidu.com",
	"xueshu.baidu.com",
	"cloud.baidu.com",
	"www.163.com",
	"open.163.com",
	"auto.163.com",
	"gov.163.com",
	"money.163.com",
	"sports.163.com",
	"tech.163.com",
	"edu.163.com",
	"www.taobao.com",
	"q.taobao.com",
	"sf.taobao.com",
	"yun.taobao.com",
	"baoxian.taobao.com",
	"www.tmall.com",
	"suning.tmall.com",
	"www.tencent.com",
	"www.qq.com",
	"www.aliyun.com",
	"www.ctrip.com",
	"hotels.ctrip.com",
	"hotels.ctrip.com",
	"vacations.ctrip.com",
	"flights.ctrip.com",
	"trains.ctrip.com",
	"bus.ctrip.com",
	"car.ctrip.com",
	"piao.ctrip.com",
	"tuan.ctrip.com",
	"you.ctrip.com",
	"g.ctrip.com",
	"lipin.ctrip.com",
	"ct.ctrip.com"
};

static void dns_async_client_result_callback(struct dns_item *list, int count) {

	int i = 0;

	for (i = 0; i < count; i++) {
		printf("name: %s, ip: %s\n", list[i].domain, list[i].ip);
	}

}

int main(int argc, char const *argv[])
{
	struct async_context *ctx = dns_async_client_init();
	if (ctx == NULL) {
		printf("dns_async_client_init\n");
		return -1;
	}

	int i = 0;
	int count = sizeof(domain) / sizeof(domain[0]);

	for (i = 0; i < count; i++) {
		dns_async_client_commit(ctx, domain[i], dns_async_client_result_callback);
	}

	getchar();
	
	return 0;
}
