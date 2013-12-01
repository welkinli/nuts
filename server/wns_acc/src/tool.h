#ifndef _TOOL_H_
#define _TOOL_H_
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <arpa/inet.h>
#include <sstream>
#include <stdio.h>

typedef struct TOOLtag
{
	static unsigned long long Int2Long(unsigned num1,unsigned num2)
	{
		return (unsigned long long)((unsigned long long)num1<<32|num2);
	}
	
	static unsigned long long Int2Long(int num1,int num2)
	{
		return (unsigned long long)((unsigned long long)num1<<32|num2);
	}
	
	static unsigned long long Int2Long(unsigned num1,unsigned short num2)
	{
		return (unsigned long long)((unsigned long long)num1<<32|(unsigned long long)num2);
	}
	
	static unsigned long long Int2Long(unsigned num1,short num2,short num3)
	{
		unsigned _tmp_num = (num2<<16)|num3;
		
		return Int2Long(num1,_tmp_num);
	}
	
	static unsigned Long2IntL(unsigned long long num)
	{
		return (unsigned)(num>>32);
	}
	
	static unsigned Long2IntR(unsigned long long num)
	{
		return (unsigned)(num&0xFFFFFFFF);
	}
	
	static void Long2Int(unsigned long long num,unsigned& num1,short& num2,short& num3)
	{
		num1 = num>>32;
		unsigned _tmp_num = num&0x00FFFFFFFF;
		num2 = (short)_tmp_num>>16;
		num3 = (short)_tmp_num&0xFFFF;
	}
	
	static unsigned long long Int2Long(int num1,short num2)
	{
		return (unsigned long long)((unsigned long long)num1<<32| (int)num2);
	}
	
	static short Long2ShortL(unsigned long long num)
	{
		return (short)((num>>32)&0xFFFF);
	}
	
	static short Long2ShortR(unsigned long long num)
	{
		return (short)(num&0xFFFF);
	}
	
	static unsigned Str2Int(char* host_addr)
	{
		return (unsigned)inet_addr(host_addr);
	};
	
    static unsigned int Str2Int(string& strInput)
    {
        stringstream ss;
        unsigned int iRet;
        ss<<strInput;
        ss>>iRet;
        return iRet;
    }
	
	static char* Int2Str(unsigned host_addr)
	{
		struct in_addr in;
		in.s_addr = host_addr;
		return (char*)inet_ntoa(in);
	};
	
	static void Int2Str(int num1,int num2,string& out_str)
	{
		char _tmp[64] = {0};
		snprintf(_tmp,sizeof(_tmp),"%d_%d",num1,num2);
		out_str = _tmp;
		
		return ;
	}
	
	static void Int2Str(unsigned host_ip,short host_port,string& host_str)
	{
		char _tmp[64] = {0};
		snprintf(_tmp,sizeof(_tmp),"%u_%u",host_ip,host_port);
		host_str = _tmp;
		
		return ;
	};
	
	static unsigned SockAddr2Ip(struct sockaddr_in sock_addr)
	{
		return (unsigned)sock_addr.sin_addr.s_addr;
	}
	
	static char* SockAddr2IpStr(struct sockaddr_in sock_addr)
	{
		struct in_addr in;
		in.s_addr = (unsigned)sock_addr.sin_addr.s_addr;
		return (char*)inet_ntoa(in);
	}
	
	static short SockAddr2Port(struct sockaddr_in sock_addr)
	{
		return ntohs(sock_addr.sin_port);
	}
	
	static void Ip2SockAddr(unsigned ip,short port,struct sockaddr_in& sock_addr)
	{
		sock_addr.sin_addr.s_addr = ip;
		sock_addr.sin_port = port;
	}
	
	static struct sockaddr_in Ip2SockAddr(char* ip,unsigned short port)
	{
		struct sockaddr_in _sock_addr;
		_sock_addr.sin_addr.s_addr = Str2Int(ip);
		_sock_addr.sin_port = port;
		return _sock_addr;
	}
	
	static void SockAddr2Str(struct sockaddr_in& sock_addr,string& host_str)
	{
		host_str.reserve(64);host_str = "";
		int _ret = snprintf((char*)host_str.c_str(),63,"%u%d",
				(unsigned)sock_addr.sin_addr.s_addr,
				sock_addr.sin_port);
		if ( _ret > 0 )*((char*)host_str.c_str() + _ret) = '\0';
		
		return ;
	}
	
	static void Int2Str(unsigned num1,unsigned num2,string& str_num)
	{
		char _tmp[64] = {0};
		snprintf(_tmp,sizeof(_tmp),"%u_%u",num1,num2);
		str_num = _tmp;
		
		return ;
	}
	
	static void Time2str(struct timeval tm_value,string& tm_str)
	{
		struct tm _tm;
		time_t now;
		now = tm_value.tv_sec;
		localtime_r(&now, &_tm);
		
		char _tm_char[32] = {0};
		snprintf(_tm_char,32,"%04d-%02d-%02d %02d:%02d:%02d",
						1900+_tm.tm_year,_tm.tm_mon+1,_tm.tm_mday,
						_tm.tm_hour,_tm.tm_min,_tm.tm_sec);
		tm_str = _tm_char;
		
		return ;
	}

	static string Time2str(struct timeval tm_value)
	{
		struct tm _tm;
		time_t now;
		now = tm_value.tv_sec;
		localtime_r(&now, &_tm);
		
		char _tm_char[32] = {0};
		snprintf(_tm_char,32,"%04d-%02d-%02d %02d:%02d:%02d",
						1900+_tm.tm_year,_tm.tm_mon+1,_tm.tm_mday,
						_tm.tm_hour,_tm.tm_min,_tm.tm_sec);
		
		return _tm_char;
	}
	
	static int Net2Ip(string net_name,string& net_ip)
	{
		#define BUFSIZE 1024
		int _sock_fd;
		struct ifconf conf;
		struct ifreq *ifr;		
		char buff[BUFSIZE];		
		int num;
		int i;
		
		_sock_fd = socket(PF_INET, SOCK_DGRAM, 0);
		if ( _sock_fd < 0 )	return -1;
		
		conf.ifc_len = BUFSIZE;
		conf.ifc_buf = buff;
		
		if ( ioctl(_sock_fd, SIOCGIFCONF, &conf) < 0 )
		{
			close(_sock_fd);
			return -1;
		}
		
		num = conf.ifc_len / sizeof(struct ifreq);
		ifr = conf.ifc_req;
		
		for( i=0; i<num;i++ )
		{
			struct sockaddr_in *sin = (struct sockaddr_in *)(&ifr->ifr_addr);
			
			if ( ioctl(_sock_fd, SIOCGIFFLAGS, ifr) < 0 )
			{
				close(_sock_fd);
				return -1;
			}
			
			if ( (ifr->ifr_flags & IFF_UP) && strcmp(net_name.c_str(),ifr->ifr_name) == 0 )
			{
				net_ip = inet_ntoa(sin->sin_addr);
				close(_sock_fd);
				
				return 0;
			}
			ifr++;
		}
		close(_sock_fd);
		
		return -1;
	}
	
	static int Net2Ip(string net_name,unsigned& net_ip)
	{
		#define BUFSIZE 1024
		int _sock_fd;
		struct ifconf conf;
		struct ifreq *ifr;		
		char buff[BUFSIZE];	
		int num;
		int i;
		
		_sock_fd = socket(PF_INET, SOCK_DGRAM, 0);
		if ( _sock_fd < 0 )	return -1;
		
		conf.ifc_len = BUFSIZE;
		conf.ifc_buf = buff;
		
		if ( ioctl(_sock_fd, SIOCGIFCONF, &conf) < 0 )
		{
			close(_sock_fd);
			return -1;
		}
		
		num = conf.ifc_len / sizeof(struct ifreq);
		ifr = conf.ifc_req;
		
		for( i=0; i<num;i++ )
		{
			struct sockaddr_in *sin = (struct sockaddr_in *)(&ifr->ifr_addr);
			
			if ( ioctl(_sock_fd, SIOCGIFFLAGS, ifr) < 0 )
			{
				close(_sock_fd);
				return -1;
			}
			
			if ( (ifr->ifr_flags & IFF_UP) && strcmp(net_name.c_str(),ifr->ifr_name) == 0 )
			{
				net_ip = (unsigned)sin->sin_addr.s_addr;
				close(_sock_fd);
				
				return 0;
			}
			ifr++;
		}
		close(_sock_fd);
		
		return -1;
	}
	
}TOOL;

#endif
