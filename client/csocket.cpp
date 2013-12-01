#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include "csocket.h"

#define _REENT_ONLY


CSocket::CSocket() 
{
    m_iSocket=-1; 
    m_iStatus=SOCK_STATUS_INVALID; 
}

CSocket::~CSocket()
{
    Close();
}


int CSocket:: Create(int nSocketType)
{
	if (m_iStatus != SOCK_STATUS_INVALID)
		return m_iSocket;
	m_iSocket = socket(AF_INET, nSocketType, 0);
	if (m_iSocket == -1)
	{
		return -1;
	}
	m_iStatus = SOCK_STATUS_ACTIVE;

	return m_iSocket;
}

int CSocket:: Connect(const char *szServerIP, u_short nServerPort)
{
	if (m_iStatus != SOCK_STATUS_ACTIVE)
	{
		Close();  // 只是为了确保不会有socket泄漏
		Create(); // 如果socket还没有创建，就调用Virtual Create()创建
	}
	struct sockaddr_in serv_addr;
	socklen_t addr_len;
	int retval;

	bzero(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	inet_aton(szServerIP, &serv_addr.sin_addr);
	serv_addr.sin_port = htons(nServerPort);
	addr_len = sizeof(serv_addr);

	retval= connect(m_iSocket, (struct sockaddr *)&serv_addr, addr_len);
	return retval;
}

/*
@param szServerIP  server host ip
@param nServerPort server listen port
@param timeout     connection timeout setting.
				   if NULL, block wait
				   if NOT NULL, non block wait
@retval -10 socket Not ready.
		-2  connect time out
		-3  connect socket exception
		-1  connect error (Connection refused, etc.)
 */
int CSocket:: Connect(const char *szServerIP, u_short nServerPort, struct timeval *timeout)
{
	if (m_iStatus != SOCK_STATUS_ACTIVE)
	{
		Close();  // 只是为了确保不会有socket泄漏
		Create(); // 如果socket还没有创建，就调用Virtual Create()创建
	}
	struct sockaddr_in serv_addr;
	socklen_t addr_len;
	int retval=0;

	bzero(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	inet_aton(szServerIP, &serv_addr.sin_addr);
	serv_addr.sin_port = htons(nServerPort);
	addr_len = sizeof(serv_addr);
	if (timeout == NULL) // 阻塞
	{
		retval= connect(m_iSocket, (struct sockaddr *)&serv_addr, addr_len);
	}
	else // 非阻塞超时
	{
		SetNonBlockOption(true);
		retval= connect(m_iSocket, (struct sockaddr *)&serv_addr, addr_len);
		if (retval== -1)
		{
			if ((errno== EINPROGRESS) || (errno== EWOULDBLOCK))
			{
				fd_set recv_fds, send_fds, exce_fds;
				int iNum;

				FD_ZERO( &recv_fds );
				FD_SET( m_iSocket, &recv_fds );
				send_fds= recv_fds;
				FD_ZERO( &exce_fds );
				FD_SET( m_iSocket, &exce_fds );
				iNum= select( m_iSocket+1, &recv_fds, &send_fds, &exce_fds, timeout );
				if (iNum <= 0) {
					retval = -2; // timeout
				}
				else if ((!FD_ISSET(m_iSocket, &recv_fds))&& (!FD_ISSET(m_iSocket, &send_fds)))
				{
					retval = -3; // connect出现错误
				}
				else
				{
					int sock_err= 0;
					socklen_t sock_err_len= (socklen_t)sizeof(sock_err);
					retval= GetSockOpt(SOL_SOCKET, SO_ERROR, &sock_err, &sock_err_len);
					if (retval == 0) {
						if (sock_err != 0)
						{
							retval= -1;
						}
					}
				}
			}
		}
		SetNonBlockOption(false);
	}
	return retval;
}

int CSocket:: Close()
{
	if (m_iSocket != -1)
	{
		close(m_iSocket);
		m_iSocket = -1;
		m_iStatus = SOCK_STATUS_INVALID;
	}

	return 0;
}

int CSocket:: SetSocket(int iSocket, int iStatus)
{
	m_iSocket = iSocket;
	m_iStatus = iStatus;
	return 0;
}

int CSocket:: Accept(CTCPSocket &client)
{
	int iSocket;
	struct sockaddr_in addr;
	socklen_t len;

	bzero(&addr, sizeof(addr));
	len = sizeof(addr);
	iSocket = accept(m_iSocket, (struct sockaddr*)&addr, &len);
	if (iSocket < 0) return -1;

	client.SetSocket(iSocket, SOCK_STATUS_CONNECT);
	return 0;
}

int CSocket:: Read(char *chBuffer, unsigned int nSize)
{
	if ( chBuffer == NULL /*|| nSize > SSIZE_MAX*/ )
		return -2; // read() will return -1 on error

	return read(m_iSocket, chBuffer, nSize); // 如果m_iSocket不合法，read会返回-1，errno为EBADF
}

int CSocket:: ReadLine(char *chBuffer, unsigned int nSize)
{
	size_t n, rc;
	char  c, *ptr;

	ptr = chBuffer;
	for (n=1; n<nSize; n++)
	{
again:
		if ((rc = read(m_iSocket, &c, 1)) == 1)
		{
			*ptr++ = c;
			if (c == '\n')
				break;
		}
		else if (rc == 0)
		{
			if (n == 1)
				return 0;
			else
				break;
		}
		else
		{
			if (errno == EINTR)
				goto again;
			return -1;
		}
	}
	*ptr=0;
	return n;
}

int CSocket:: Send(const void* chBuff, size_t nSize, int flags)
{
	return send(m_iSocket, chBuff, nSize, flags);
}
int CSocket:: Write(const char *chBuffer, unsigned int nSize)
{
	return write(m_iSocket, chBuffer, nSize);
}

int CSocket:: WriteLine(const char *chBuffer)
{
	return write(m_iSocket, chBuffer, strlen(chBuffer));
}

int CSocket:: Bind(const char *pszBindIP, const unsigned short iBindPort)
{
	int iRetCode = 0;
	struct sockaddr_in bind_addr;

	if (m_iStatus != SOCK_STATUS_ACTIVE) // socket has not create
		return -1;
	bzero(&bind_addr, sizeof(bind_addr));
	bind_addr.sin_family = AF_INET;
//	inet_pton(AF_INET, pszBindIP, &bind_addr.sin_addr);
	bind_addr.sin_addr.s_addr = inet_addr(pszBindIP);

	bind_addr.sin_port = htons(iBindPort);

	iRetCode = bind(m_iSocket, (struct sockaddr *)&bind_addr, sizeof(bind_addr));
	if (iRetCode == 0)
		m_iStatus = SOCK_STATUS_BIND;
	return iRetCode;
}

int CSocket:: Listen(const int iBackLog)
{
	int iRetCode = 0;

	if (m_iStatus != SOCK_STATUS_BIND)
		return -1;

	iRetCode = listen(m_iSocket, iBackLog);
	if (iRetCode == 0)
		m_iStatus = SOCK_STATUS_LISTEN;
	return iRetCode;
}

int CSocket:: Recvfrom(void *pBuffer, 
                            size_t nBytes, 
                            int iFlags, 
                            struct sockaddr *pFromAddr, 
                            socklen_t *iAddrLen)
{
	return recvfrom(m_iSocket, pBuffer, nBytes, iFlags, pFromAddr, iAddrLen);
}

int CSocket:: Sendto(void *pBuffer, 
                            size_t nBytes, 
                            int iFlags, 
                            struct sockaddr *pToAddr, 
                            socklen_t &iAddrLen)
{
	return sendto(m_iSocket, pBuffer, nBytes, iFlags, pToAddr, iAddrLen);
}

int CSocket:: GetSockOpt(int level, 
                            int optname,
                            void *optval,
                            socklen_t *optlen)
{
	return ::getsockopt(m_iSocket, level, optname, optval, optlen);
}

int CSocket:: SetSockOpt(int level, 
                            int optname, 
                            const void *optval,
                            socklen_t optlen)
{
	return ::setsockopt(m_iSocket, level, optname, optval, optlen);
}

int CSocket:: SetNonBlockOption(bool flag)
{
	int save_mode;
	save_mode = fcntl( m_iSocket, F_GETFL, 0 );
	if (flag) { // set nonblock
		save_mode |= O_NONBLOCK;
	}
	else { // set block
		save_mode &= (~O_NONBLOCK);
	}
	fcntl( m_iSocket, F_SETFL, save_mode );

	return 0;
}

int CSocket:: GetSockName(struct sockaddr *name, socklen_t *namelen)
{
	return getsockname(m_iSocket, name, namelen);
}

int CSocket:: GetPeerName(struct sockaddr *name, socklen_t *namelen)
{
	return getpeername(m_iSocket, name, namelen);
}

int CSocket:: Flush(int iFlag)
{
	return ioctl(m_iSocket, I_FLUSH, iFlag);
}

int CSocket:: RecvWithTimeout(void *pBuffer, size_t nBytes, struct timeval *timeout)
{
	int iNum= 0;

	if ( nBytes<=0 ) return 0;

	iNum= SelectRead (timeout);
	if (iNum <= 0) { return iNum; }

	iNum = Read ((char*)pBuffer, nBytes);
	return iNum;
}

int CSocket:: SelectRead (struct timeval* timeout)
{
	fd_set recv_fds;
	int iNum= 0;

	if (m_iSocket <0) return -1;

	FD_ZERO( &recv_fds );
	FD_SET( m_iSocket, &recv_fds );

	iNum= select( m_iSocket+1, &recv_fds, NULL, NULL, timeout );
	return iNum;
}

int CSocket:: RecvExact(void *pBuffer, size_t nBytes, struct timeval *timeout)
{
	int iNum;
	size_t nRecvBytes=0;
	char *pRecvBuf= (char*)pBuffer;

	if ( nBytes<=0 ) return 0;

	while (nRecvBytes < nBytes)
	{
		iNum= SelectRead (timeout);

		if (iNum < 0) { // on error
			return iNum;
		}
		else if (iNum == 0) { // timeout
			break;
		}

		iNum = Read((char*)pRecvBuf, nBytes-nRecvBytes);
		if (iNum > 0) {
			nRecvBytes += iNum;
			pRecvBuf += iNum;
		}
		else if (iNum == 0) { // connection closed
			break;
		}
		else
		{ // onerror
			if (errno == EWOULDBLOCK)
			{ // timeout
				break;
			}
		}
	}
	return nRecvBytes;
}

////////////////////////////////////////////////////////
//
//  TCP Socket
////////////////////////////////////////////////////////
CTCPSocket::CTCPSocket()
    : CSocket()
{ // 因为Accept时会由accept()返回一个socket，所以在TCPSocket构造时不要Create()
}

int CTCPSocket::Create()
{
	return CSocket::Create(SOCK_STREAM);
}


////////////////////////////////////////////////////////
//
//  UDP Socket
////////////////////////////////////////////////////////
int CUDPSocket::Create()
{
	return CSocket::Create(SOCK_DGRAM);
}

CUDPSocket::CUDPSocket()
    : CSocket()
{
	Create();
}


