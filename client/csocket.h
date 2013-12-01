#if !defined _CSOCKET_H_
#define _CSOCKET_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stropts.h>

#define SOCK_STATUS_INVALID		-1
#define SOCK_STATUS_CONNECT		2
#define SOCK_STATUS_LISTEN		3
#define SOCK_STATUS_ACTIVE		4
#define SOCK_STATUS_BIND		5

class CTCPSocket;
class CUDPSocket;

/**
 * CTCPSocket/CUDPSocket的基类
 *
 * 实现connect/send/recv/accept/bind/listen/sendto/recvfrom封装;
 * 对有超时控制的方法，因为内部使用select控制超时，
 * 所以socket有1024的限制；
 *
 */
class CSocket
{
public:
    /**
     * 构造函数
     */
	CSocket();

    /**
     * 析构函数
     */
	virtual ~CSocket();

public:
    /**
     * 创建socket，派生类实现
     */
	virtual int Create()= 0;

    /**
     * 创建特定类型的socket
     *
     * @param nSocketType SOCK_STREAM or SOCK_DGRAM
     * @return -1:      失败
     *         其他:    socket句柄值 
     */
	int Create(int nSocketType);


    /**
     * 连接指定（IP、Port）的机器
     *
     * 阻塞连接
     *
     * @param szServerIP 需要连接的IP
     * @param iServerPort 需要连接的Port
     * @return 0:       成功
     *         -1：     失败，详细错误码见errno
     * 
     */
	int Connect(const char *szServerIP, 
                    u_short iServerPort);

    /**
     * 连接指定（IP、Port）的机器
     *
     * 允许超时控制, 内部用select控制超时
     *
     * @param szServerIP 需要连接的IP
     * @param iServerPort 需要连接的Port
     * @param timeout 超时设置，
     *          如果为NULL，阻塞连接，直至成功或失败返回；
     *          如果不为NULL，阻塞连接，直至成功或失败或超时返回
     *              
     * @return -2: connect time out
     * @return -3: connect socket exception
     * @return -1: connect error (Connection refused, etc.)
     */
    int Connect(const char *szServerIP, 
                    u_short iServerPort, 
                    struct timeval *timeout);

    /**
     *  关闭socket
     *
     *  @return 0 
     */
	int Close();


    /**
     * read封装函数
     *
     * @param chBuffer 读取的数据存放buffer
     * @param nSize chBuffer大小
     * @return 0: 对端关闭连接
     * @return -1: read失败，详细错误信息见errno
     * @return -2: 参数非法
     * @return >0: 读取到的实际数据长度
     *
     * @see ReadLine
     * @see RecvExact
     * @see RecvWithTimeout
     */
	int Read(char *chBuffer, unsigned int nSize);

    /**
     * read一行数据(行结束标识字符：\n)
     *
     * @param chBuffer 读取的数据存放buffer
     * @param nSize chBuffer大小
     * @return -1: read失败，详细错误信息见errno
     * @return 其他: 读取到的实际数据长度, 
     *         当对端关闭连接时，立即返回当前已经读取长度
     *
     * @see Read
     * @see RecvExact
     * @see RecvWithTimeout
     */
	int ReadLine(char *chBuffer, unsigned int nSize);


	/**
     * send封装方法
     *
     * 增加send以支持flags方式的发送,详见 man send
     *
     * @param chBuff 待发送的数据buffer
     * @param nSize 待发送的数据长度
     * @param flags 详见 man send
     * @return -1: send失败，详细错误信息见errno
     * @return 其他：实际发送的数据长度
     *
     * @see Write
     * @see WriteLine
     */
	int Send(const void* chBuff, size_t nSize, int flags);

    /**
     * write封装方法
     *
     * @param chBuffer 待发送数据buffer
     * @param nSize 待发送数据长度
     * @return -1: write失败，详细错误信息见errno
     * @return 其他：实际发送的数据长度
     *
     * @see Send
     * @see WriteLine
     */
	int Write(const char *chBuffer, unsigned int nSize);

    /**
     * write以\\0字符结尾的字符串
     *
     * @param chBuffer 待发送字符串
     * @return -1: write失败，详细错误信息见errno
     * @return 其他：实际发送的数据长度
     *
     * @see Write
     * @see Send
     */
	int WriteLine(const char *chBuffer);

    /**
     * bind指定（ip，port）
     *
     * @param pszBindIP 待绑定ip
     * @param iBindPort 待绑定port
     * @return -1: 绑定失败
     * @return 0：绑定成功
     *
     */
	int Bind(const char *pszBindIP, const unsigned short iBindPort);

    /**
     * listen封装
     *
     * listen之前，必须先调用Bind
     *
     * @param iBackLog 见man listen
     * @return 0： 成功
     * @return -1：失败
     *
     * @see Bind
     *
     */
	int Listen(const int iBackLog);

    /**
     * recvfrom封装
     *
     * @param pBuffer 接收数据buffer
     * @param nBytes 接收数据buffer大小
     * @param iFlags recvfrom flag
     * @param pFromAddr remote peer address
     * @param iAddrLen remote peer address length
     * @return >0: 接收到数据长度，
     * @return 0： 无数据或remote peer orderly shutdown；
     * @return -1：失败，错误码errno
     */
	int Recvfrom(void *pBuffer, 
                    size_t nBytes, 
                    int iFlags, 
                    struct sockaddr *pFromAddr,
                    socklen_t* iAddrLen);

    /**
     * sendto封装
     *
     * @param pBuffer 发送数据buffer
     * @param nBytes 发送数据大小
     * @param iFlags sendto flag
     * @param pToAddr 目标地址
     * @param iAddrLen 目标地址长度
     * @return >=0: 实际发送的数据长度; 
     * @return -1：失败，错误码errno
     */
	int Sendto(void *pBuffer, 
                    size_t nBytes, 
                    int iFlags, 
                    struct sockaddr *pToAddr, 
                    socklen_t &iAddrLen);

    /**
     * getsockopt封装
     *
     * 详细信息参见man getsockopt
     *
     */
	int GetSockOpt(int level, 
                        int optname,
                        void *optval, 
                        socklen_t *optlen);

    /**
     * setsockopt封装
     *
     * 详细信息参见man setsockopt
     *
     */
	int SetSockOpt(int level, 
                        int optname, 
                        const void *optval, 
                        socklen_t optlen);

    /**
     * 检查socket是否可读？可以超时控制
     *
     * SelectRead之前，必须将socket设置为非阻塞模式
     *
     * @param timeout 超时时间
     * @return 0: 成功，可读
     *         -1：失败
     *
     * @see SetNonBlockOption
     *
     */
	int SelectRead (struct timeval* timeout);

    /**
     * 获取本地地址
     *
     * @param name 本地地址
     * @param namelen 本地地址长度
     * 
     * @return 0: 成功
     *         -1：失败
     */
	int GetSockName(struct sockaddr *name, socklen_t *namelen);

    /**
     * 获取对端地址
     *
     * @param name 对端地址
     * @param namelen 对端地址长度
     * 
     * @return 0: 成功
     *         -1：失败
     */
	int GetPeerName(struct sockaddr *name, socklen_t *namelen);

	/** 
     * flush socket I/O 
     *
     * @param iFlag man 2 ioctl I_FLUSH
     * @return 详细见man 2 ioctl I_FLUSH
     */
	int Flush(int iFlag= FLUSHRW);

	/**
     * set non-block option to socket
     *
     * @param flag true to set non-block option; false to set block option
     * @return 0
     *
     */
	int SetNonBlockOption(bool flag);

	/**
     * read from socket with timeout 
     *
     * @param pBuffer 接收buffer
     * @param nBytes 接收buffer大小
     * @param timeout 超时时间
     *
     * @return -1: read失败，可能是超时
     * @return 其他： 读取的实际数据长度
     *
     * @see RecvExact
     * @see SetNonBlockOption
     */
	int RecvWithTimeout(void *pBuffer, size_t nBytes, struct timeval *timeout);

	/** 
     * read exactly N bytes
     *
     * @param pBuffer 接收buffer
     * @param nBytes 接收buffer大小
     * @param timeout 超时时间
     * @return -1: read失败，可能是超时
     * @return 其他： 读取的实际数据长度
     * 
     * @see RecvWithTimeout
     * @see SetNonBlockOption
     */
	int RecvExact(void *pBuffer, size_t nBytes, struct timeval *timeout);

    /**
     * 绑定外部socket
     *
     * @param iSocket 外部socket句柄
     * @param iStatus 初始状态：
     *              SOCK_STATUS_CONNECT/SOCK_STATUS_BIND/SOCK_STATUS_LISTEN/SOCK_STATUS_ACTIVE
     *
     * @return 0
     */
	int SetSocket(int iSocket, int iStatus);

    /**
     * accept封装
     *
     * accept的fd绑定到CTCPSocket对象
     *
     * @param client 将accept的fd绑定到此对象
     * @return 0： 成功
     * @return -1：失败
     *
     */
	int Accept(CTCPSocket &client);

protected:
	int m_iSocket;
	int m_iStatus;
};

/**
 * tcp socket
 *
 */
class CTCPSocket 
    : public CSocket
{
public:
    /**
     * 构造函数
     */
	CTCPSocket();

    /**
     * 析构函数
     */
	virtual ~CTCPSocket() {
		Close();
	};
    /**
     * 创建特定类型的socket
     *
     * @return -1:      失败
     *         其他:    socket句柄值 
     */
	virtual int Create();
};

/**
 * udp socket
 *
 */
class CUDPSocket 
    : public CSocket
{
public:
    /**
     * 构造函数
     */
	CUDPSocket();
    /**
     * 析构函数
     */
	virtual ~CUDPSocket() {
		Close();
	};
    /**
     * 创建特定类型的socket
     *
     * @return -1:      失败
     *         其他:    socket句柄值 
     */
	virtual int Create();
};

#endif // _CSOCKET_H_


