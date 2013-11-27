include $(PATH_QZONE_MOBILE_FRAMEWORK_PRJ)/plib/makefile.plib

CC = g++

DEFINES = -D_GNU_SOURCE -D__STDC_LIMIT_MACROS -DPRINT_MM_DBG=1
DEBUG_FLAGS = -DDEBUG=1

#CCFLAGS = -g -Wall -m32 -DSUS_LINUX -Wno-sign-compare -Wno-unused -O2
CCFLAGS = -g -Wall -m32 -DSUS_LINUX -Wno-sign-compare -Wno-unused $(DEBUG_FLAGS)
SHARED_LIB_FLAGS = -fPIC -shared

INCLUDE = -I. -I../../lib/elib -I../../lib/misclib -I../../lib/mylib -I../../lib/dencode -I../../lib/3rd_party/libevent/include -I../../lib/3rd_party/glib -I../../lib/vrb_lib/include -I/usr/local/include

LDFLAGS = -L. -L../../lib/misclib -lmisc -L../../lib/elib -lelib -L../../lib/mylib -L/usr/local/lib -lcrypt -L../../lib/3rd_party/libevent/lib -levent -L../../lib/3rd_party/glib -lglib-2.0 -lgthread-2.0 -lz -lrt -ldl -L../../lib/vrb_lib/lib -lvrb -lpthread

GLIB_FLAGS = `pkg-config --cflags --libs glib-2.0`
GTHREAD_FLAGS = `pkg-config --cflags --libs gthread-2.0`
GMODULE_FLAGS = `pkg-config --cflags --libs gmodule-2.0`

TEST_MM = test_malloc
TEST_BUDDY = test_buddy

TEST_MM_SOURCES = test_malloc.c test_malloc.h
TEST_BUDDY_SOURCES = test_buddy.c test_buddy.h

BIN = $(TEST_MM) $(TEST_BUDDY)

all:	$(BIN)

check:
	$(PATH_QZONE_MOBILE_FRAMEWORK_PRJ)/makeinclude/make_check ./ 

%.o : %.c
	$(CC) $(DEFINES) $(CCFLAGS) $(INCLUDE) -c $<  

$(TEST_MM):	$(TEST_MM_SOURCES)
	$(CC) $(DEFINES) $(CCFLAGS) $(INCLUDE) -o $(TEST_MM) $(TEST_MM_SOURCES) $(LDFLAGS)

$(TEST_BUDDY):	$(TEST_BUDDY_SOURCES)
	$(CC) $(DEFINES) $(CCFLAGS) $(INCLUDE) -o $(TEST_BUDDY) $(TEST_BUDDY_SOURCES) $(LDFLAGS)

clean:
	rm -fr $(BIN) *.o core core.* *~ *.so *.so.* 

distclean:	clean
	rm -fr ../log/* ../bin/core ../bin/core.*
