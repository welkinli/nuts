CC = g++
AR = ar rvs
CFLAGS = -Wall -Wno-unused -g -O2 -pipe -D_GNU_SOURCE
BIN = libmisclib.so
BIN_A = libmisclib.a
SOURCES = misc_cfg.c misc_str.c
HEADERS = misc_cfg.h  misc_str.h
OBJS = misc_cfg.o misc_str.o

INCLUDE = -I. -I.. -I../misclib -I../mylib

LDFLAGS = -L../misclib -lmisc

all:    $(BIN_A)

%.o : %.c
	$(CC) $(INCLUDE) $(CFLAGS) -c $<

$(BIN_A) : $(OBJS)
	$(AR) $@ $(OBJS)

clean:
	rm -fr *.o $(OBJS) core core.* *~ $(BIN) $(BIN_A)
