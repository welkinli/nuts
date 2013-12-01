#include "csocket.h"
#define stx 0x74756e
#if defined(__linux__)
#  include <endian.h>
#elif defined(__FreeBSD__) || defined(__NetBSD__)
#  include <sys/endian.h>
#elif defined(__OpenBSD__)
#  include <sys/types.h>
#  define be16toh(x) betoh16(x)
#  define be32toh(x) betoh32(x)
#  define be64toh(x) betoh64(x)
#endif
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
int main(int c,char **s){
	CTCPSocket csocket;
	csocket.SetNonBlockOption(true);
	if(csocket.Connect("10.12.18.103",14000)){
		printf("connect error!\n");
	}else{
		printf("connect ok!\n");
	}
	char buff[1024]={0};
	unsigned long long_value=0;
	unsigned int int_value=0;
	unsigned int short_value=0;
	char char_value=0;
	int pos=0;

	int_value=htonl(stx);
	memcpy(buff,&int_value,sizeof(int_value));
	pos+=sizeof(int_value);

	int_value=htonl(39);
	memcpy(buff,&int_value,sizeof(int_value));
	pos+=sizeof(int_value);

	*(buff+pos)=1;
	pos++;

	int_value=htonl(1000);
	memcpy(buff,&int_value,sizeof(int_value));
	pos+=sizeof(int_value);

	long_value=55056676;	
	memcpy(buff,&long_value,sizeof(long_value));
	pos+=sizeof(long_value);

	long_value=12132543;	
	memcpy(buff,&long_value,sizeof(long_value));
	pos+=sizeof(long_value);

	int_value=htonl(0);
	memcpy(buff,&int_value,sizeof(int_value));
	pos+=sizeof(int_value);

	short_value=htons(0);
	memcpy(buff,&short_value,sizeof(short_value));
	pos+=sizeof(short_value);

	int send_len=(csocket.Send(buff,pos,0));
	printf("send len:%d\n",send_len);
	if(send_len<0){
		printf("send len <0,error:%d,info:%s\n",errno,strerror(errno));
		return -1;
	}else{
		printf("send ok,send len:%d\n",send_len);
	}
	char recvbuff[1024]={0};
	struct timeval timeout;
	timeout.tv_sec=1;
	timeout.tv_usec=0;
	int recv_len=csocket.RecvWithTimeout(recvbuff,39,&timeout);
	if(recv_len>0)
	{
		printf("recv head ok,recv:%d\n",recv_len);
		int total=ntohl(*(recvbuff+4));
		int bodylen=csocket.Read(recvbuff+recv_len,total-recv_len);
		if(bodylen>0){
			printf("recv body ok,len:%d\n",bodylen);
		}
	}else{
		if(recv_len ==0)
		printf("server close connection!\n");
		else
		printf("recv len <0,recv_len:%d,error:%d,info:%s\n",recv_len,errno,strerror(errno));
	}
	csocket.Close();
	return 0;
}
