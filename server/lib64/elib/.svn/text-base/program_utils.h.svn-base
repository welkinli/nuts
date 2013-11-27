#ifndef	__PROGRAM_UTILS_H_
#define	__PROGRAM_UTILS_H_	1

#include	"common_albert.h"
#include	"albert_log.h"
#include	<net/if.h>
#include	<netinet/in.h>
#include	<arpa/inet.h>
#include	<sys/ioctl.h>
#include	<error.h>

#ifdef	__cplusplus
extern "C" {
#endif

#if __BYTE_ORDER == __BIG_ENDIAN
#define ntohll64(x)       (x)
#define htonll64(x)       (x)
#else
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define ntohll64(x)     __bswap_64 (x)
#define htonll64(x)     __bswap_64 (x)
#endif
#endif

#define	NETWORK2HOST_64(v)		(host64_t)(ntohll64(v))
#define	NETWORK2HOST_32(v)		(host32_t)(ntohl(v))
#define	NETWORK2HOST_16(v)		(host16_t)(ntohs(v))
#define	HOST2NETWORK_64(v)		(network32_t)(htonll64(v))
#define	HOST2NETWORK_32(v)		(network32_t)(htonl(v))
#define	HOST2NETWORK_16(v)		(network16_t)(htons(v))

#define	GET_STRING_NOVAL_8(s, l)	do {(l) = *((uint8_t *)(s)); (s) += sizeof(uint8_t); (s) += (l);} while (0)
#define	GET_STRING_VAL_8(s, l, p)	do {(l) = *((uint8_t *)(s)); (s) += sizeof(uint8_t); (p) = (s); (s) += (l);} while (0)
#define	GET_STRING_NOVAL_16(s, l)	do {(l) = *((uint16_t *)(s)); (s) += sizeof(uint16_t); (s) += (l);} while (0)
#define	GET_STRING_VAL_16(s, l, p)	do {(l) = *((uint16_t *)(s)); (s) += sizeof(uint16_t); (p) = (s); (s) += (l);} while (0)
#define	GET_STRING_NOVAL_32(s, l)	do {(l) = *((uint32_t *)(s)); (s) += sizeof(uint32_t); (s) += (l);} while (0)
#define	GET_STRING_VAL_32(s, l, p)	do {(l) = *((uint32_t *)(s)); (s) += sizeof(uint32_t); (p) = (s); (s) += (l);} while (0)

#define	GET_STRING_NOVAL_16_CONV(s, l)	do {(l) = NETWORK2HOST_16(*((uint16_t *)(s))); (s) += sizeof(uint16_t); (s) += (l);} while (0)
#define	GET_STRING_VAL_16_CONV(s, l, p)	do {(l) = NETWORK2HOST_16(*((uint16_t *)(s))); (s) += sizeof(uint16_t); (p) = (s); (s) += (l);} while (0)
#define	GET_STRING_NOVAL_32_CONV(s, l)	do {(l) = NETWORK2HOST_32(*((uint32_t *)(s))); (s) += sizeof(uint32_t); (s) += (l);} while (0)
#define	GET_STRING_VAL_32_CONV(s, l, p)	do {(l) = NETWORK2HOST_32(*((uint32_t *)(s))); (s) += sizeof(uint32_t); (p) = (s); (s) += (l);} while (0)

#define	SET_STRING_VAL_8(s, l, p)		do {*((uint8_t *)(s)) = (l); (s) += sizeof(uint8_t); memcpy((s), (p), (l)); (s) += (l);} while (0)
#define	SET_STRING_VAL_8_CONV(s, l, p)		do {*((uint8_t *)(s)) = (l); (s) += sizeof(uint8_t); memcpy((s), (p), (l)); (s) += (l);} while (0)
#define	SET_STRING_VAL_16(s, l, p)	do {*((uint16_t *)(s)) = (l); (s) += sizeof(uint16_t); memcpy((s), (p), (l)); (s) += (l);} while (0)
#define	SET_STRING_VAL_16_CONV(s, l, p)	do {*((uint16_t *)(s)) = HOST2NETWORK_16((l)); (s) += sizeof(uint16_t); memcpy((s), (p), (l)); (s) += (l);} while (0)
#define	SET_STRING_VAL_32(s, l, p)	do {*((uint32_t *)(s)) = (l); (s) += sizeof(uint32_t); memcpy((s), (p), (l)); (s) += (l);} while (0)
#define	SET_STRING_VAL_32_CONV(s, l, p)	do {*((uint32_t *)(s)) = HOST2NETWORK_32((l)); (s) += sizeof(uint32_t); memcpy((s), (p), (l)); (s) += (l);} while (0)

#define	GET_STRING_VAL_NO_LEN(s, l, p)	do {memcpy((p), (s), (l)); (s) += (l);} while (0)
#define	SET_STRING_VAL_NO_LEN(s, l, p)	do {memcpy((s), (p), (l)); (s) += (l);} while (0)


#define	STRING_OCCUPY_LEN_8(l)		(sizeof(uint8_t) + (l))
#define	STRING_OCCUPY_LEN_16(l)		(sizeof(uint26_t) + (l))
#define	STRING_OCCUPY_LEN_32(l)		(sizeof(uint32_t) + (l))
#define	STRING_OCCUPY_LEN_64(l)		(sizeof(uint64_t) + (l))

#define	GET_8(s, l)	do {(l) = *((uint8_t *)(s)); (s) += sizeof(uint8_t); } while (0)
#define	GET_8_CONV(s, l)	do {(l) = *((uint8_t *)(s)); (s) += sizeof(uint8_t); } while (0)
#define	GET_16(s, l)	do {(l) = *((uint16_t *)(s)); (s) += sizeof(uint16_t); } while (0)
#define	GET_16_CONV(s, l)	do {(l) = NETWORK2HOST_16(*((uint16_t *)(s))); (s) += sizeof(uint16_t); } while (0)
#define	GET_32(s, l)	do {(l) = *((uint32_t *)(s)); (s) += sizeof(uint32_t); } while (0)
#define	GET_32_CONV(s, l)	do {(l) = NETWORK2HOST_32(*((uint32_t *)(s))); (s) += sizeof(uint32_t); } while (0)
#define	GET_64(s, l)	do {(l) = *((uint64_t *)(s)); (s) += 8; } while (0)
#define	GET_64_CONV(s, l)	do {(l) = NETWORK2HOST_64(*((uint64_t *)(s))); (s) += sizeof(uint64_t); } while (0)
#define	GET_64_HISTORY(s, l)	do {\
								char _tmp_buf[8]; \
								_tmp_buf[0] = (s)[3]; _tmp_buf[1] = (s)[2]; _tmp_buf[2] = (s)[1]; _tmp_buf[3] = (s)[0]; _tmp_buf[4] = (s)[7]; _tmp_buf[5] = (s)[6]; _tmp_buf[6] = (s)[5]; _tmp_buf[7] = (s)[4]; \
								(l) = *((uint64_t *)_tmp_buf); \
								(s) += 8; \
							} while (0)

#define	SET_8(s, l)	do {*((uint8_t *)(s)) = (l); (s) += sizeof(uint8_t); } while (0)
#define	SET_8_CONV(s, l)	do {*((uint8_t *)(s)) = (l); (s) += sizeof(uint8_t); } while (0)
#define	SET_16(s, l)	do {*((uint16_t *)(s)) = (l); (s) += sizeof(uint16_t); } while (0)
#define	SET_16_CONV(s, l)	do {*((uint16_t *)(s)) = HOST2NETWORK_16((l)); (s) += sizeof(uint16_t); } while (0)
#define	SET_32(s, l)	do {*((uint32_t *)(s)) = (l); (s) += sizeof(uint32_t); } while (0)
#define	SET_32_CONV(s, l)	do {*((uint32_t *)(s)) = HOST2NETWORK_32((l)); (s) += sizeof(uint32_t); } while (0)
#define	SET_64(s, l)	do {*((uint64_t *)(s)) = (l); (s) += 8; } while (0)
#define	SET_64_CONV(s, l)	do {*((uint64_t *)(s)) = HOST2NETWORK_64((l)); (s) += sizeof(uint64_t); } while (0)
#define	SET_64_HISTORY(s, l)	do {\
								char *_tmp_buf = (char *)(&(l)); \
								(s)[0] = _tmp_buf[3]; (s)[1] = _tmp_buf[2]; (s)[2] = _tmp_buf[1]; (s)[3] = _tmp_buf[0]; (s)[4] = _tmp_buf[7]; (s)[5] = _tmp_buf[6]; (s)[6] = _tmp_buf[5]; (s)[7] = _tmp_buf[4]; \
								(s) += 8; \
							} while (0)

/* safe strlen */
#define	strlens(s)		((s) ? strlen((s)) : 0)

/* safe set value */
#define	set_pval(p, v)	do {if ((p)) {*(p) = (v);}} while (0)
#define	set_pval_type(p, v, t)	do {if ((p)) {*(p) = (t)(v);}} while (0)

/* copy data to array */
#define	copy_to_array(a, l, s)	do {uint32_t _ml = MIN((l), (sizeof((a)) - 1)); if (_ml > 0) memcpy((a), (s), _ml); ((a)[_ml] = '\0')} while (0)

#ifndef	dlmalloc
#define	dlmalloc(l)	malloc((l))
#define	dlcalloc(a, b)	calloc((a), (b))
#define	dlrealloc(p, s)	realloc((p), (s))
#define	dlfree(p)		free((p))
#endif

#define	dlmalloc_ptr(p, l)	do {(p) = dlmalloc((l)); if (!(p)) {g_crit("dlmalloc() failed: %s", strerror(errno)); (p) = NULL;}} while (0)
#define	dlfree_ptr(p)	do {if ((p)) {dlfree((p)); (p) = NULL;}} while (0)
#define	malloc_ptr(p, l)	do {(p) = malloc((l)); if (!(p)) {g_crit("malloc() failed: %s", strerror(errno)); (p) = NULL;}} while (0)
#define	free_ptr(p)	do {if ((p)) {free((p)); (p) = NULL;}} while (0)
#define	delete_ptr(p)	do {if (p) {delete (p); (p) = NULL;}} while (0)

#define	dlmalloc_a(t, p, l)	do {void *_p = NULL; dlmalloc_ptr(_p, (l)); (p) = (t *)(_p);} while (0)
#define	new_a(t, p, l)	do {void *_p = NULL; new_ptr(_p, (l)); (p) = (t *)(_p);} while (0)

#ifdef	PRINT_MM_WARN
#define	dlmalloc_dbg(p, l, t)	do {(p) = (t)dlmalloc((l)); if (!(p)) {g_warn("dlmalloc() failed: %s", strerror(errno)); (p) = NULL;} else {g_warn("MM dlmalloc: %p", (p));}} while (0)
#define	dlfree_dbg(p)	do {if ((p)) {g_warn("MM dlfree ptr: %p", (p)); dlfree((p)); (p) = NULL;}} while (0)
#define	malloc_dbg(p, l, t)	do {(p) = (t)malloc((l)); if (!(p)) {g_warn("malloc() failed: %s", strerror(errno)); (p) = NULL;} else {g_warn("MM malloc: %p", (p));}} while (0)
#define	realloc_dbg(n, p, l, t)	do {(n) = (t)realloc((p), (l)); if (!(n)) {g_warn("realloc() failed: %s", strerror(errno)); (n) = NULL;} else {g_warn("MM realloc: %p from %p", (n), (p));}} while (0)
#define	free_dbg(p)	do {if ((p)) {g_warn("MM free ptr: %p", (p)); free((p)); (p) = NULL;}} while (0)
#define	new_dbg(p, l)	do {(p) = new (l); if (!(p)) {g_warn("new() failed: %s", strerror(errno)); (p) = NULL;} else {g_warn("MM new: %p", (p));}} while (0)
#define	new_dbg_2(p, l, a)	do {(p) = new (l)(a); if (!(p)) {g_warn("new() failed: %s", strerror(errno)); (p) = NULL;} else {g_warn("MM new: %p", (p));}} while (0)
#define	delete_dbg(p)	do {if ((p)) {g_warn("MM delete ptr: %p", (p)); delete (p); (p) = NULL;}} while (0)
#else

  #ifdef	PRINT_MM_DBG
#define	dlmalloc_dbg(p, l, t)	do {(p) = (t)dlmalloc((l)); if (!(p)) {g_warn("dlmalloc() failed: %s", strerror(errno)); (p) = NULL;} else {g_dbg("MM dlmalloc: %p", (p));}} while (0)
#define	dlfree_dbg(p)	do {if ((p)) {g_dbg("MM dlfree ptr: %p", (p)); dlfree((p)); (p) = NULL;}} while (0)
#define	malloc_dbg(p, l, t)	do {(p) = (t)malloc((l)); if (!(p)) {g_warn("malloc() failed: %s", strerror(errno)); (p) = NULL;} else {g_dbg("MM malloc: %p", (p));}} while (0)
#define	realloc_dbg(n, p, l, t)	do {(n) = (t)realloc((p), (l)); if (!(n)) {g_warn("realloc() failed: %s", strerror(errno)); (n) = NULL;} else {g_dbg("MM realloc: %p from %p", (n), (p));}} while (0)
#define	free_dbg(p)	do {if ((p)) {g_dbg("MM free ptr: %p", (p)); free((p)); (p) = NULL;}} while (0)
#define	new_dbg(p, l)	do {(p) = new (l); if (!(p)) {g_warn("new() failed: %s", strerror(errno)); (p) = NULL;} else {g_dbg("MM new: %p", (p));}} while (0)
#define	new_dbg_2(p, l, a)	do {(p) = new (l)(a); if (!(p)) {g_warn("new() failed: %s", strerror(errno)); (p) = NULL;} else {g_dbg("MM new: %p", (p));}} while (0)
#define	delete_dbg(p)	do {if ((p)) {g_dbg("MM delete ptr: %p", (p)); delete (p); (p) = NULL;}} while (0)
  #else
#define	dlmalloc_dbg(p, l, t)	do {(p) = (t)dlmalloc((l)); if (!(p)) {(p) = NULL;} else {}} while (0)
#define	dlfree_dbg(p)	do {if ((p)) {dlfree((p)); (p) = NULL;}} while (0)
#define	malloc_dbg(p, l, t)	do {(p) = (t)malloc((l)); if (!(p)) {(p) = NULL;} else {}} while (0)
#define	realloc_dbg(n, p, l, t)	do {(n) = (t)realloc((p), (l)); if (!(n)) {(n) = NULL;} else {}} while (0)
#define	free_dbg(p)	do {if ((p)) {free((p)); (p) = NULL;}} while (0)
#define	new_dbg(p, l)	do {(p) = new (l); if (!(p)) {(p) = NULL;} else {}} while (0)
#define	new_dbg_2(p, l, a)	do {(p) = new (l)(a); if (!(p)) {(p) = NULL;} else {}} while (0)
#define	delete_dbg(p)	do {if ((p)) {delete (p); (p) = NULL;}} while (0)
  #endif

#endif

typedef int (*file_init_func)(int fd, void *arg);

extern int readn(int fd, char *buf, uint32_t size);
extern int writen(int fd, char *buf, uint32_t size);
extern int sendn(int fd, void *buf, uint32_t size, int flag);
extern int lock_a_file(int fd, pid_t pid);
extern int unlock_a_file(int fd, pid_t pid);
extern int uin2path(char *buf, uint32_t uin, const char *prefix);
//extern int make_and_open_file(const char *path, file_init_func func, void *arg);
//extern int open_create_file(const char *path, file_init_func func, void *arg);
extern void reset_timeval_timeout(struct timeval *tvp, uint32_t sec, uint32_t usec);
extern uint32_t time_duration(struct timeval *end, struct timeval *start);
extern void msec2timeval(struct timeval *tvp, long msec);
extern void *map_file_in(const char *file_path, uint32_t len, off_t offset, int *fdp, FILE *err_fp, int *existp);
extern int map_file_out(void *address, uint32_t len);
extern int get_interfaces(int sockfd, struct ifconf *ifc);
extern struct sockaddr_in *get_if_addr(char *iface);
extern int copy_to_pointer(void **ptr, uint32_t len, void *data, void *(*malloc_func)(size_t), void (*free_func)(void *));
extern int chars_to_hex(const char * data, int len, char *out_buf, int out_buf_len);
extern int chars_to_hex_low(const char * data, int len, char *out_buf, int out_buf_len);
/*
  * return >= 0: success, result length;
  *	       < 0:	failed
  */
extern int hex_to_chars(const char* source_data, int source_len, char* res_result, int res_len);

extern void to64frombits(unsigned char *out, const unsigned char *in, int inlen);
extern int from64tobits(char *out, const char *in);

extern ssize_t my_readline(int fd, void *vptr, size_t maxlen);

extern char *prou_hex_dump_str(char *data, uint32_t size);
extern char *prou_hex_data(char *data, uint32_t size);
extern int prou_dump_to_file(char *path, char *ptr, uint32_t len, int verbose);

extern int common_log_init(const char *path, CRollLog **logp);
extern void common_log_destroy(CRollLog **logp);
extern int bit_test(int32_t *Base, int Offset);

#ifdef	__cplusplus
}
#endif

#endif

