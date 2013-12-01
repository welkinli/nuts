SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp = .o)
CC = g++ -g
INCLUDES = -I./
LIBS =
CCFLAGS = -g -Wall -O0

client: $(OBJS)
	$(CC) $^ -o $@ $(INCLUDES) $(LIBS)

%.o : %.c
	$(CC) -c $< $(CCFLAGS)
%.o : %.cpp
	$(CC) -c $< $(CCFLAGS)
clean:
	rm *.o
