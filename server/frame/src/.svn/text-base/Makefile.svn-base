CC = g++

DEFINES = -D_GNU_SOURCE 

CCFLAGS = -g -Wall -Wno-unused -O3 -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free
SHARED_LIB_FLAGS = -fPIC -shared

INCLUDE = -I../../lib64/elib -I../../lib64/misclib -I../../lib64/libevent/include 

LDFLAGS = -L../../lib64/misclib -lmisclib \
		  -L../../lib64/elib -lelib \
		  -L../../lib64/libevent/lib -levent \
		  -L../../lib64/jemalloc/lib -ljemalloc \
		  -lz -lrt -ldl -rdynamic -lpthread

PROG = ../bin/eservice_framework

OBJ = eservice_main.o eservice_worker.o eservice_network.o eservice_utils.o eservice_buf.o
HEAD = eservice_main.h eservice_worker.h eservice_network.h eservice_utils.h eservice_buf.h

all:  $(PROG) 

check:
	$(PATH_QZONE_MOBILE_FRAMEWORK_PRJ)/makeinclude/make_check ./ 

%.o : %.cpp
	$(CC) $(DEFINES) $(CCFLAGS) $(INCLUDE) -c $<  

%.o : %.c
	$(CC) $(DEFINES) $(CCFLAGS) $(INCLUDE) -c $<  

$(PROG):	$(HEAD) $(OBJ)
	$(CC) $(DEFINES) $(CCFLAGS) $(INCLUDE) -o $@ $(OBJ) $(LDFLAGS)

clean:
	rm -fr $(PROG) $(USERSO) $(OBJ) *.o core core.* *~ *.so *.so.* 

distclean:	clean
	rm -fr ../log/* ../bin/core ../bin/core.*

