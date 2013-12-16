#include "csocket.h"
#define stx 0x74756e
#define PT_SWAP_DDWORD(x)  \
    ((((x) & 0xff00000000000000llu) >> 56) | \
     (((x) & 0x00ff000000000000llu) >> 40) | \
     (((x) & 0x0000ff0000000000llu) >> 24) | \
     (((x) & 0x0000ff0000000000llu) >> 24) | \
     (((x) & 0x000000ff00000000llu) >> 8) | \
     (((x) & 0x00000000ff000000llu) << 8) | \
     (((x) & 0x0000000000ff0000llu) << 24) | \
     (((x) & 0x000000000000ff00llu) << 40) | \
     (((x) & 0x00000000000000ffllu) << 56) )

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define PT_HTONLL(x) PT_SWAP_DDWORD(x)
#else
#define PT_HTONLL(x) (x)
#endif

#ifndef htonll
#define htonll(x) PT_HTONLL(x)
#endif

#ifndef ntohll
#define ntohll(x) htonll(x)
#endif
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
int main(int c, char **s) {
	CTCPSocket csocket;
	csocket.SetNonBlockOption(true);
	if (csocket.Connect("10.12.18.103", 14000)) {
		printf("connect error!\n");
	} else {
		printf("connect ok!\n");
	}
	char buff[1024] = { 0 };
	unsigned long long_value = 0;
	unsigned int int_value = 0;
	unsigned int short_value = 0;
	char char_value = 0;
	int pos = 0;

	int_value = htonl(stx);
	memcpy(buff+pos, &int_value, sizeof(int_value));
	pos += sizeof(int_value);

	int_value = htonl(37);
	memcpy(buff+pos, &int_value, sizeof(int_value));
	pos += sizeof(int_value);

	*(buff + pos) = 1;
	pos++;

	int_value = htonl(1000);
	memcpy(buff+pos, &int_value, sizeof(int_value));
	pos += sizeof(int_value);

	long_value = htonll(55056676);
	memcpy(buff+pos, &long_value, sizeof(long_value));
	pos += sizeof(long_value);

	long_value = htonll(12132543);
	memcpy(buff+pos, &long_value, sizeof(long_value));
	pos += sizeof(long_value);

	int_value = htonl(0);
	memcpy(buff+pos, &int_value, sizeof(int_value));
	pos += sizeof(int_value);

	short_value = htons(0);
	memcpy(buff+pos, &short_value, sizeof(short_value));
	pos += sizeof(short_value);

	do {
		int send_len = (csocket.Send(buff, pos, 0));
		printf("send len:%d\n", send_len);
		if (send_len < 0) {
			printf("send len <0,error:%d,info:%s\n", errno, strerror(errno));
			return -1;
		} else {
			printf("send ok,send len:%d\n", send_len);
		}
		char recvbuff[1024] = { 0 };
		struct timeval timeout;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		int recv_len = csocket.RecvWithTimeout(recvbuff, 39, &timeout);
		if (recv_len > 0) {
			printf("recv head ok,recv:%d\n", recv_len);
			int total = ntohl(*(recvbuff+4));
			int bodylen = csocket.Read(recvbuff + recv_len, total - recv_len);
			if (bodylen > 0) {
				printf("recv body ok,len:%d\n", bodylen);
			}
		} else {
			if (recv_len == 0){
				printf("server close connection!\n");
			}
			else
				printf("recv len <0,recv_len:%d,error:%d,info:%s\n", recv_len,
						errno, strerror(errno));
			break;
		}
		sleep(30);
	} while (1);
	csocket.Close();
	return 0;
}
