CC = g++

DEFINES = -D_GNU_SOURCE
#DEBUG_FLAGS = -DDEBUG=1 -DESERVICE_SINGLE_OUTPUT=1 -Ddebug_log=g_dbg
PLUGIN_INC_LIB_PATH = ../../
CCFLAGS = -g -Wall -Wno-unused -fPIC -O2 \
		  -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free
#CCFLAGS = -g -Wall -O2 -m32 -DSUS_LINUX -Wno-unused $(DEBUG_FLAGS)
SHARED_LIB_FLAGS = -shared

INCLUDE = \
		  -I$(PLUGIN_INC_LIB_PATH)frame/include \
		  -I$(PLUGIN_INC_LIB_PATH)lib64/qmfproto/include \
		  -I$(PLUGIN_INC_LIB_PATH)lib64/qmfproto_app/include \
		  -I$(PLUGIN_INC_LIB_PATH)frame/src/ \
		  -I$(PLUGIN_INC_LIB_PATH)lib64/elib \
		  -I$(PLUGIN_INC_LIB_PATH)lib64/misclib \
		  -I$(PLUGIN_INC_LIB_PATH)lib64/libevent/include \
		  -I$(PLUGIN_INC_LIB_PATH)lib64/vaskey/include \
		  -I$(PLUGIN_INC_LIB_PATH)lib64/fileconfig/include \
		  -I$(PLUGIN_INC_LIB_PATH)lib64/dcapi/include \
		  -I$(PLUGIN_INC_LIB_PATH)lib64/tea/include \
		  -I$(PLUGIN_INC_LIB_PATH)lib64/sac/include \

LDFLAGS = \
		  -L$(PLUGIN_INC_LIB_PATH)lib64/fileconfig/lib -lfileconfig \
		  -L$(PLUGIN_INC_LIB_PATH)lib64/dcapi/lib -ldcapi_cpp-64\
		  -L$(PLUGIN_INC_LIB_PATH)lib64/qmfproto/lib -lqmfproto -lsnappy \
		  -L$(PLUGIN_INC_LIB_PATH)lib64/qmfproto_app/lib -lqmfproto_app \
		  -L$(PLUGIN_INC_LIB_PATH)lib64/tea/lib -ltea\
		  -L$(PLUGIN_INC_LIB_PATH)lib64/vaskey/lib -lvas_key \
		  -lz 

USERSO = ../../frame/lib/libplugin.so
USERSO_OBJS = plugin.o

all:	$(USERSO)

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
