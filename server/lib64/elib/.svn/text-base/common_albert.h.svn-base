#ifndef	__COMMON_H_albert
#define	__COMMON_H_albert	1

#include	<stdio.h>
#include	<stdlib.h>
#include	<stdarg.h>
#include	<unistd.h>
#include	<string.h>
#include	<errno.h>
#include	<ctype.h>
#include	<fcntl.h>
#include	<signal.h>
#include	<time.h>
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<sys/time.h>
#include	<sys/mman.h>
#include	<sys/resource.h>
#include	<sys/wait.h>
#include	<arpa/inet.h>
#include	<netinet/in.h>
#include	<netinet/ip.h>
#include	<netinet/ip_icmp.h>
#include	<netinet/tcp.h>
#include	<netinet/udp.h>
#include	<linux/stddef.h>	/* for "offsetof" */
#include	<pthread.h>

#ifndef	__STDC_LIMIT_MACROS
#define	__STDC_LIMIT_MACROS	1
#endif

#include	<stdint.h>

#define	PAGE_SIZE	4096

#ifndef NUL
#define NUL '\0'
#endif
#ifndef NULL
#define NULL (void *)0
#endif

/* bitmask generation */
#define V_BIT(n) (1<<(n))

#ifndef	MAX
#define	MAX(x, y)	((x) > (y) ? (x) : (y))
#define	MIN(x, y)		((x) < (y) ? (x) : (y))
#endif

#ifndef	ylog
#define	ylog_force(fp, str, arg...)	do {if ((ylog_level >= 0)) {fprintf((fp), "(%s: %u) "str, __FILE__, __LINE__, ##arg);}} while (0)
#define	ylog_pos(fp, str, arg...)		do {if ((ylog_level >= 1)) {fprintf((fp), "(%s: %u) "str, __FILE__, __LINE__, ##arg);}} while (0)
#define	ylog(fp, str, arg...)		do {if ((ylog_level >= 2)) {fprintf((fp), str, ##arg);}} while (0)
#endif

#ifndef	g_warn
#define	G_ALOG_LEVEL_NONE	0
#define	G_ALOG_LEVEL_WARN	1
#define	G_ALOG_LEVEL_MSG	2
#define	G_ALOG_LEVEL_DBG	3

#define g_dbg(arg1, arg2...)	do {if (g_log_level >= G_ALOG_LEVEL_DBG) {GDEBUG_LOG(arg1, ##arg2);}} while (0)
#define g_msg(arg1, arg2...)		do {if (g_log_level >= G_ALOG_LEVEL_MSG) {GNOTI_LOG(arg1, ##arg2);}} while (0)
#define g_warn(arg1, arg2...)		do {if (g_log_level >= G_ALOG_LEVEL_WARN) {GERROR_LOG("(%s: %u) "arg1, __FILE__, __LINE__, ##arg2);}} while (0)
#define g_crit(arg1, arg2...)   do {if (g_log_level >= G_ALOG_LEVEL_NONE) {GERROR_LOG("(%s: %u) "arg1, __FILE__, __LINE__, ##arg2);}} while (0)
#define g_err(arg1, arg2...)   do {if (g_log_level >= G_ALOG_LEVEL_NONE) {GABORT_LOG("(%s: %u) "arg1, __FILE__, __LINE__, ##arg2);}} while (0)

#endif

#define	INIT_PROCESS_PID	1
#define	MAX_MLINE_LEN	4096
#define	MAX_ALBERT_PATH_LEN	1024

#define	PIPE_BUF_ATOMIC	512
#define	MAX_BUFSIZE	65536
#define	MAX_LINE_LEN	1024
#define	MAX_CONF_LEN	256
#define	ADDITIONAL_SAFE_LEN	1024

typedef	uint64_t	network64_t;
typedef	uint32_t	network32_t;
typedef	uint16_t	network16_t;
typedef	uint64_t	host64_t;
typedef	uint32_t	host32_t;
typedef	uint16_t	host16_t;

#define	ADBG_DEBUG	"XXX DEBUG: "
#define	ADBG_ERROR	"XXX ERROR: "
#define	ADBG_NS	"XXX NOT SUPPORTED: "

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif	/* !__COMMON_H_albert */

