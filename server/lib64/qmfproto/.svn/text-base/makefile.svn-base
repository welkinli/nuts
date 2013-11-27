SRC_DIR=./src/
OBJ_DIR=
LIB_DIR=./lib/

OBJ_EXT=.o
CPPSRC_EXT=.cpp
CSRC_EXT= .c

INC = -I./include/ -I../pdu/include -I../qmfproto_app/include -I../tea/include -I../vaskey/include

OBJECTS=./qmf_service_define.o ./CQmfCryptor.o ./CQmfCompress.o ./reportLog.o ./WnsQuaHelper.o

LIB_TARGET=./lib/libqmfproto.a

C_FLAGS = -g -Wall -D_GNU_SOURCE -fPIC -O2

$(OBJ_DIR)%$(OBJ_EXT):$(SRC_DIR)%$(CPPSRC_EXT)
	@echo -e "\033[1;33m\nCompiling $< ==> $@ \033[0m..."
	$(CXX) $(INC) $(C_FLAGS) -c $< -o $@
	@echo ""
	
$(OBJ_DIR)%$(OBJ_EXT):$(SRC_DIR)%$(CSRC_EXT)
	@echo -e "\033[1;33m\nCompiling $< ==> $@ \033[0m..."
	$(CXX) $(INC) $(C_FLAGS) -c $< -o $@
	@echo ""

$(LIB_TARGET):$(OBJECTS)
	@echo "$(OBJECTS) ==> $@"
	$(AR) rc $(LIB_TARGET) $(OBJECTS)
	rm $(OBJECTS) -f
	@echo ""

CHECK:
	$(PATH_QZONE_MOBILE_FRAMEWORK_PRJ)/makeinclude/make_check

all: CHECK $(LIB_TARGET)

clean:
	rm $(OBJECTS) $(LIB_TARGET) -rf
	
	
