#ifndef	G_LOG_DOMAIN
#define	G_LOG_DOMAIN	"program_utils"
#endif

#include	"program_utils.h"

extern uint32_t g_log_level;
extern CRollLog *g_albert_logp;			/* extern log */

int
readn(int fd, char *buf, uint32_t size)
{
	int already = 0;
	int n;

	while (already < (int)size) {
		n = read(fd, buf + already, size - already);
		if (n < 0) {
			return -1;
		}
		if (n == 0) {
			return already;
		}

		already += n;
	}
	
	return (int)size;
}

int
writen(int fd, char *buf, uint32_t size)
{
	int already = 0;
	int n;

	while (already < (int)size) {
		n = write(fd, buf + already, size - already);
		if (n < 0) {
			return already;
		}

		already += n;
	}
	
	return (int)size;
}

int
sendn(int fd, void *buf, uint32_t size, int flag)
{
	int already = 0;
	int n;

	while (already < (int)size) {
		n = send(fd, (char *)buf + already, size - already, flag);
		if (n < 0) {
			return already;
		}

		already += n;
	}
	
	return (int)size;
}

int
lock_a_file(int fd, pid_t pid) {
	struct flock lock_struct;
	
	memset(&lock_struct, 0, sizeof(lock_struct));
	lock_struct.l_type = F_WRLCK;
	lock_struct.l_whence = SEEK_SET;
	lock_struct.l_start = 0;
	lock_struct.l_len = 0;
	lock_struct.l_pid = pid;
	if (fcntl(fd, F_SETLKW, &lock_struct) < 0) {
		fprintf(stderr, "fcntl lock failed: %s\n", strerror(errno));
		return -1;
	}

	return 0;
}

int
unlock_a_file(int fd, pid_t pid) {
	struct flock lock_struct;
	
	memset(&lock_struct, 0, sizeof(lock_struct));
	lock_struct.l_type = F_UNLCK;
	lock_struct.l_whence = SEEK_SET;
	lock_struct.l_start = 0;
	lock_struct.l_len = 0;
	lock_struct.l_pid = pid;
	if (fcntl(fd, F_SETLKW, &lock_struct) < 0) {
		fprintf(stderr, "fcntl lock failed: %s\n", strerror(errno));
		return -1;
	}
	
	return 0;
}

int
uin2path(char *buf, uint32_t uin, const char *prefix)
{
	char tmp[16];
	int base = 0, i;
	uint32_t n, num = uin;
	
	memset(tmp, '0', 16);
	tmp[14] = '/';
	tmp[15] = '\0';
	i = 13;

	n = 1;
	while (i > 0) {
		if (num == 0)
			n = 0;
		else
			n = num % 10;

		tmp[i] = '0' + n;
		i--;
		num /= 10;
		base++;

		switch(base) {
			case 3:
			case 5:
			case 7:
			case 10:
				tmp[i] = '/';
				i--;
				break;
			default:
				break;
		}
	}

	snprintf(buf, 1024, "%s%s", prefix, tmp);
	
	return 0;
}

#if 0
int
make_and_open_file(const char *path, file_init_func init_func, void *arg)
{
	char *lpath = strdup(path);
	char *ptr = 0;
	int res;
	char cmd[64];

	ptr = strrchr(lpath, '/');
	if (!ptr) {
		fprintf(stderr, "make_file: invalid path: %s\n", path);
		return -1;
	}

	*ptr = '\0';
	snprintf(cmd, sizeof(cmd), "mkdir -p %s", lpath);

	res = system(cmd);
	if (res < 0) {
		fprintf(stderr, "system() failed: %s\n", strerror(errno));
		return -1;
	}
	
	res = open(path, O_RDWR | O_CREAT | O_EXCL, 0666);
	if (res < 0) {
		fprintf(stderr, "create file: %s failed: %s\n", path, strerror(errno));
		return -1;
	}
	
	if (init_func) {
		if(init_func(res, arg)) {
			fprintf(stderr, "user init func failed\n");
			return -1;
		}
	}

	/* rewind to beginning */
	lseek(res, 0, SEEK_SET);
	
	return res;
}

int
open_create_file(const char *path, file_init_func init_func, void *arg)
{
	int fd;

	fd = open(path, O_RDWR);
	if (fd < 0) {
		if (errno != ENOENT) {
			fprintf(stderr, "open() failed: %s\n", strerror(errno));
			return -1;
		}

		if ((fd = make_and_open_file(path, init_func, arg)) < 0) {
			fprintf(stderr, "make_and_open_file() failed\n");
			return -2;
		}
	}
	
	return fd;
}
#endif

void
reset_timeval_timeout(struct timeval * tvp, uint32_t sec, uint32_t usec)
{
	if (!tvp)
		return;
	
	tvp->tv_sec = sec;
	tvp->tv_usec = usec;
	return;
}

uint32_t
time_duration(struct timeval *end, struct timeval *start)
{
	long sec, msec;
	uint32_t ti = 0;

	sec = end->tv_sec - start->tv_sec;
	msec = end->tv_usec - start->tv_usec;

	if (sec < 0)
		return 0;
	
	while (msec < 0) {
		sec--;
		msec += 1000000;

		if (sec < 0)
			return 0;
	}

	ti = sec * 1000000 + msec;
	
	return ti;
}

void
msec2timeval(struct timeval *tvp, long msec)
{
	tvp->tv_sec = msec / 1000;
	tvp->tv_usec = (msec % 1000) * 1000;

	return;
}

void *
map_file_in(const char *file_path, uint32_t len, off_t offset, int *fdp, FILE *err_fp, int *existp)
{
	void *address = NULL;
	int fd = -1;
	int exists = 0;
	struct stat the_stat;

	if (!fdp || *fdp < 0) {
		if (stat(file_path, &the_stat) < 0) {
			if (errno == ENOENT) {
				fd = open(file_path, O_RDWR | O_CREAT, 0666);
				if (fd < 0) {
					g_crit("open() failed: %s", strerror(errno));
					return NULL;
				}

				g_dbg("%s not exists, create it", file_path);
				exists = 0;
				close(fd);
			}
			else
				GABORT_LOG("stat() for %s failed: %s", file_path, strerror(errno));
		}
		else {
			g_dbg("%s does exists", file_path);
			exists = 1;
		}

		fd = open(file_path, O_RDWR);
		if (fd < 0) {
			g_crit("open() failed: %s", strerror(errno));
			return NULL;
		}

		if (exists == 0) {
			lseek(fd, len - 1, SEEK_SET);
			write(fd, "", 1);
		}
		else {
			if (stat(file_path, &the_stat) < 0)
				g_warn("stat() for %s failed: %s", file_path, strerror(errno));
			else {
				if (the_stat.st_size != (off_t)len) {
					g_warn("file length(%ld) mismatchs wanted length(%u)", the_stat.st_size, len);
					close(fd);
					return NULL;
				}
			}
		}
	}
	else {
		exists = 1;
		fd = *fdp;
	}
	
	address = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset);
	if (!address) {
		g_crit("mmap() failed: %s", strerror(errno));
		close(fd);
		return NULL;
	}

	if (fdp)
		*fdp = fd;
	else
		close(fd);
	
	if (exists == 0) {
		memset(address, '\n', len);
	}

	if (existp)
		*existp = exists;

	return address;
}

int
map_file_out(void *address, uint32_t len)
{
	return munmap(address, len);
}

int
get_interfaces(int sockfd, struct ifconf *ifc)
{
  int      len, lastlen;
  char      *buf;

  /* Grab a list of interfaces */
  
  lastlen = 0;
  len = 20 * sizeof(struct ifreq);  /* initial buffer size guess */
  for ( ; ; ) {
    buf = (char *)malloc(len);

    ifc->ifc_len = len;
    ifc->ifc_buf = buf;
    if (ioctl(sockfd, SIOCGIFCONF, ifc) < 0) {
      if (errno != EINVAL || lastlen != 0) {
	error(0, 1, "ioctl(SIOCGIFCONF)\n");
	return -1;
      }
    } else {
      if (ifc->ifc_len == lastlen)
	break;    /* success, len has not changed */
      lastlen = ifc->ifc_len;
    }
    len += sizeof(struct ifreq);  /* increment */
    free(buf);
  }
  
  return 0;
}

struct sockaddr_in *
get_if_addr(char *iface)
{
  int family = AF_INET;
  int      sockfd, len, flags;
  char      *ptr;
  struct ifconf    ifc;
  struct ifreq    *ifr=NULL, ifrcopy;
  struct sockaddr_in  *sinptr=NULL;

  if (!iface) {
    error(0, 0, "no inter face specified\n");
    return NULL;
  }

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    error(0, 1, "socket\n");
    return 0;
  }
  
  if (get_interfaces(sockfd, &ifc) == -1) {
    error(0, 1, "get_interfaces\n");
    return 0;
  }

  /* Walk the list of interfaces and locate the first one which is up */

  for (ptr = ifc.ifc_buf; ptr < (ifc.ifc_buf + ifc.ifc_len); ) {
    ifr = (struct ifreq *) ptr;
    
    len = sizeof(struct ifreq);
    ptr += len;  /* for next one in buffer */

    if (ifr->ifr_addr.sa_family != family)
      continue;  /* ignore if not desired address family */

    ifrcopy = *ifr;
    if (ioctl(sockfd, SIOCGIFFLAGS, &ifrcopy) < 0) {
      error(0, 1, "ioctl(SIOCGIFFLAGS)\n");
    }

    flags = ifrcopy.ifr_flags;

    if ((flags & IFF_UP) == 0)
      continue;  /* ignore if interface not up */

    if (strncmp("lo", ifr->ifr_name, 2)==0)
      continue;

    if (iface) {
      if (!strncmp(iface, ifr->ifr_name, IFNAMSIZ) == 0)
	continue; /* Not the interface we're looking for */
    }
	
    switch (ifr->ifr_addr.sa_family) {
    case AF_INET:
      sinptr = (struct sockaddr_in *) &ifr->ifr_addr;
      //printf("Local IP: %s\n", inet_ntoa(sinptr->sin_addr));
      break;

    default:
      //printf("Protocol family not supported.");
      sinptr = NULL;
    }
    break;
  }
  if (iface && (sinptr == 0)) {
    error(0, 0, "No such interface '");
    return NULL;
  }
  return sinptr;
}

/* copy data to pointer */
int 
copy_to_pointer(void **ptr, uint32_t len, void *data, void *(*malloc_func)(size_t), void (*free_func)(void *))
{
	//int res = 0;
	char *p = NULL;

	if (!ptr || !data) {
		errno = EINVAL;
		return -1;
	}

	if (free_func && !malloc_func) {
		errno = EINVAL;
		return -2;
	}
	
	if (*ptr && free_func) {
		free_func(*ptr);
		*ptr = NULL;
	}

	if (!(*ptr)) {
		*ptr = malloc_func(len + 1);
		if (!(*ptr))
			return -3;
	}
	
	memcpy(*ptr, data, len);

	p = (char *)(*ptr);
	p[len] = '\0';
	
	return 0;
}

static char HEX_DATA_MAP[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
static char HEX_DATA_MAP_LOW[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

static int 
char_to_hex(char b, char * buf)
{
	int h1, h2;
	if(buf == NULL)
		return -1;

	h1 = (b & 0xF0) >> 4;
	h2 = b & 0xF;

	buf[0] = HEX_DATA_MAP[h1];
	buf[1] = HEX_DATA_MAP[h2];

	return 0;
}

static int 
char_to_hex_low(char b, char * buf)
{
	int h1, h2;
	if(buf == NULL)
		return -1;

	h1 = (b & 0xF0) >> 4;
	h2 = b & 0xF;

	buf[0] = HEX_DATA_MAP_LOW[h1];
	buf[1] = HEX_DATA_MAP_LOW[h2];

	return 0;
}

int
chars_to_hex(const char * data, int len, char *out_buf, int out_buf_len)
{
	char buf[2];
	int i;
	
	if(!data || len <= 0)
		return -1;

	if (!out_buf || out_buf_len < 2 * len)
		return -2;
	
	memset(out_buf, 0, out_buf_len);

	for(i = 0; i < len; i++)
	{
		char_to_hex(*(data + i), buf);
		out_buf[i * 2] = buf[0];
		out_buf[i * 2 + 1] = buf[1];
	}

	return 0;
}

int
chars_to_hex_low(const char * data, int len, char *out_buf, int out_buf_len)
{
	char buf[2];
	int i;
	
	if(!data || len <= 0)
		return -1;

	if (!out_buf || out_buf_len < 2 * len)
		return -2;
	
	memset(out_buf, 0, out_buf_len);

	for(i = 0; i < len; i++)
	{
		char_to_hex_low(*(data + i), buf);
		out_buf[i * 2] = buf[0];
		out_buf[i * 2 + 1] = buf[1];
	}

	return 0;
}

/*
  * return >= 0: success, result length;
  *	       < 0:	failed
  */
int 
hex_to_chars(const char* source_data, int source_len, char* des_result, int des_len)
{
        char temp;
        char value = 0;
        int i = 0 ,j = 0;
        if (source_data == NULL || source_len <= 0 || des_result == NULL){
                return -1;
        }
        for (i = 0;i < source_len; i++){
                if (source_data[i] >= '0' && source_data[i] <= '9'){
                        temp = source_data[i] - 48;
                }

                else if (source_data[i] >= 'A' && source_data[i] <= 'F'){
                        temp = source_data[i] - 'A' + 10;
                }

                else if (source_data[i] >= 'a' && source_data[i] <= 'f'){
                        temp = source_data[i] - 'a' + 10;
                }
                else{
                    return -2;
                }
				
                if (i%2 == 0){
                        value = temp << 4;
                }
                else{
                        if (j >= des_len){
                             return -3;
                        }
                        value |= temp;
                        des_result[j++] = value;
                        value = 0;
                }
        }
        if (source_len % 2 == 1){
                if (j >= des_len){
                     return -3;
                }

                temp = 0;
                value |= temp;
                des_result[j++] = value;
                value = 0;
        }
        return j;
}

static int	read_cnt;
static char	*read_ptr;
static char	read_buf[4096];

static ssize_t
my_read(int fd, char *ptr)
{

	if (read_cnt <= 0) {
again:
		if ( (read_cnt = read(fd, read_buf, sizeof(read_buf))) < 0) {
			if (errno == EINTR)
				goto again;
			return(-1);
		} else if (read_cnt == 0)
			return(0);
		read_ptr = read_buf;
	}

	read_cnt--;
	*ptr = *read_ptr++;
	return(1);
}

ssize_t
my_readline(int fd, void *vptr, size_t maxlen)
{
	ssize_t	rc;
    size_t n;
	char	c, *ptr;

	ptr = (char *)vptr;
	for (n = 1; n < maxlen; n++) {
		if ( (rc = my_read(fd, &c)) == 1) {
			*ptr++ = c;
			if (c == '\n')
				break;	/* newline is stored, like fgets() */
		} else if (rc == 0) {
			*ptr = 0;
			return(n - 1);	/* EOF, n - 1 bytes were read */
		} else
			return(-1);		/* error, errno set by read() */
	}

	*ptr = 0;	/* null terminate like fgets() */
	return(n);
}

char* 
prou_hex_dump_str(char *data, uint32_t size)
{
    int fin = 0;
    uint32_t srcpos=0, dstpos=0;
    int linePos;
    char charBuf[16];
    uint8_t lineBuf[32];
    enum{BUFLEN=256*1024};
    static char sHexBuf[BUFLEN];

    sHexBuf[0]='\0';
    for(srcpos=0, linePos=0;dstpos<BUFLEN;srcpos++)
    {
        if(srcpos<size)
        {
            if(linePos==0){
                dstpos += snprintf(&(sHexBuf[dstpos]), BUFLEN, "%04x: ", srcpos);
            }
            snprintf(charBuf, sizeof(charBuf) - 1, "%08x", data[srcpos]);
            dstpos += snprintf(&(sHexBuf[dstpos]), sizeof(charBuf), "%c%c ", charBuf[6], charBuf[7]);
            lineBuf[linePos]=data[srcpos];
            if(lineBuf[linePos]<0x20 || lineBuf[linePos]>0x7f)
                lineBuf[linePos]='.';
        }
        else
        {
            if(linePos==0)
                break;
            else
            {
                dstpos += snprintf(&(sHexBuf[dstpos]), BUFLEN, "   ");
                lineBuf[linePos]=' ';
                fin = 1;
            }
        }
        linePos++;
        if(!(linePos<16))
        {
            lineBuf[linePos]=0;
            dstpos += snprintf(&(sHexBuf[dstpos]), BUFLEN, "%s\n", lineBuf);
            linePos=0;
            if(fin) break;
        }
    }
    sHexBuf[dstpos] = '\0';
    return sHexBuf;
}

char* 
prou_hex_data(char *data, uint32_t size)
{
    unsigned int srcpos=0, dstpos=0;
    char charBuf[16];
    static char sHexData[1024];

    sHexData[0]='\0';
    for(srcpos=0; srcpos< size&&dstpos<1024; srcpos++)
    {
        snprintf(charBuf, sizeof(charBuf) -1, "%08x", data[srcpos]);
        dstpos += snprintf(&(sHexData[dstpos]), sizeof(charBuf) -1, "%c%c ", charBuf[6], charBuf[7]);
    }
    sHexData[dstpos] = '\0';
    return sHexData;
}

int
prou_dump_to_file(char *path, char *ptr, uint32_t len, int verbose)
{
	FILE *fp = NULL;

	fp = fopen(path, "w+");
	if (!fp) {
		g_warn("fopen for dump file failed: %s: %s", path, strerror(errno));
		return -1;
	}

	if (verbose >= 1)
		fprintf(fp, "%s\n", prou_hex_dump_str(ptr, len));
	else
		fprintf(fp, "%s\n", prou_hex_data(ptr, len));

	fclose(fp);
	
	return 0;
}

int
common_log_init(const char *path, CRollLog **logp)
{
	g_dbg("common_log_init: %s", path);

	if (!logp)
		return -1;
	
	if (*logp) {
		delete_dbg(*logp);
	}
	
	new_dbg(*logp, CRollLog);
	g_dbg("run_log_file: %p", *logp);
	if (!(*logp))
		return -2;
		
	(*logp)->init(string(path), "");
	(*logp)->_flags[CRollLog::F_DEBUGTIP] = false;
	
	return 0;
}

void
common_log_destroy(CRollLog **logp)
{
	if (!logp)
		return;
	
	if (*logp) {
		delete_dbg(*logp);
	}
	
	*logp = NULL;
	
	return;
}

int bit_test(int32_t *Base, int Offset)
{
	if (((*Base>>Offset) & 0x01) == 1) {
		return 1;
	}
	else {
		return 0;
	}
}
