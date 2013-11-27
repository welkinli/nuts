CC = g++

DEFINES = -D_GNU_SOURCE -DQZONE_PROJECT_ROOT=\"$(PATH_QZONE_RUNNING)\"
#DEBUG_FLAGS = -DDEBUG=1 -DESERVICE_SINGLE_OUTPUT=1 -Ddebug_log=g_dbg

CCFLAGS = -g -Wall -Wno-unused -fPIC -O2 \
		  -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free
#CCFLAGS = -g -Wall -O2 -m32 -DSUS_LINUX -Wno-unused $(DEBUG_FLAGS)
SHARED_LIB_FLAGS = -shared

INCLUDE = \
		  -I../../lib64/pdu/include \
		  -I../../lib64/qmfproto/include \
		  -I../../lib64/qmfproto_app/include \
		  -I../../frame/src/ \
		  -I../../lib64/elib \
		  -I../../lib64/misclib \
		  -I../../lib64/libevent/include \
		  -I../../lib64/l5/include \
		  -I../../lib64/vaskey/include \
		  -I../../lib64/fileconfig/include \
		  -I../../lib64/dcapi/include \
		  -I../../lib64/tea/include \
		  -I../../lib64/sac/include \
		  -I../../lib64/pdu/include \
		  -I../../lib64/qmfproto/include \
		  -I../../lib64/qmfproto_app/include \

LDFLAGS = \
		  -L../../lib64/l5/lib -lqos_client64\
		  -L../../lib64/fileconfig/lib -lfileconfig \
		  -L../../lib64/dcapi/lib -ldcapi_cpp-64\
		  -L../../lib64/sac/lib -lsac_agent \
		  -L../../lib64/qmfproto/lib -lqmfproto -lsnappy \
		  -L../../lib64/qmfproto_app/lib -lqmfproto_app \
		  -L../../lib64/pdu/lib -lpdu_header \
		  -L../../lib64/tea/lib -ltea\
		  -L../../lib64/vaskey/lib -lvas_key \
		  -lz 

USERSO = ../lib/libwns_es_acc_plugin.so
USERSO_OBJS = wns_es_acc_plugin.o comm_dchelper.o

all:	$(USERSO)

check:
	$(PATH_QZONE_MOBILE_FRAMEWORK_PRJ)/makeinclude/make_check ./ 

%.o : %.cpp
	$(CC) $(DEFINES) $(CCFLAGS) $(INCLUDE) -c $<  

%.o : %.c
	$(CC) $(DEFINES) $(CCFLAGS) $(INCLUDE) -c $<  

$(USERSO):	$(USERSO_OBJS)
	$(CC) $(DEFINES) $(CCFLAGS) $(INCLUDE) $(SHARED_LIB_FLAGS) -o $@ $(USERSO_OBJS) $(LDFLAGS)

clean:
	rm -fr $(USERSO) $(OBJ) *.o core core.* *~ *.so *.so.* 

distclean:	clean
	rm -fr ../log/* ../lib/*
