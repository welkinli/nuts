CC = g++
AR = ar rvs
CFLAGS = -Wall -Wno-unused -g -O2 -D_GNU_SOURCE -DRELEASE=1 \
		 -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free
#CFLAGS = -Wall -Wno-unused -Wno-sign-compare -g -D_GNU_SOURCE -DDEBUG=1
#CFLAGS = -Wall -Wno-unused -Wno-sign-compare -g -D_GNU_SOURCE -DDEBUG=1 -DBUF_MM_WARN=1

INCLUDE = -I. -I.. -I../misclib -I../mylib

LDFLAGS = -L../misclib -lmisclib

BIN_SO = libelib.so
BIN_A = libelib.a

OBJS = program_utils.o socket_utils.o albert_log.o module_albert.o ss_list_utils.o

HEADERS = dbllist.h program_utils.h socket_utils.h albert_log.h module_albert.h ss_list_utils.h
 
%.o : %.c
	$(CC) $(INCLUDE) $(CFLAGS) -fPIC -c $<

%.o : %.cpp
	$(CC) $(INCLUDE) $(CFLAGS) -fPIC -c $<

#all:	$(BIN_SO) $(BIN_A)
all:	$(BIN_SO)

$(BIN_SO):	$(OBJS) $(HEADERS)
	$(CC) $(INCLUDE) $(CFLAGS) -shared -o $@ $(OBJS) $(LDFLAGS)

$(BIN_A):	$(OBJS) $(HEADERS)
	$(AR) $@ $(OBJS)
clean:
	rm -fr *.o $(BIN_SO) $(BIN_A) core core.* *~
