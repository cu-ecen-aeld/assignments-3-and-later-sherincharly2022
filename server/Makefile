TARGET=aesdsocket

CC ?= gcc 
TARGET_LDFLAGS ?= -pthread -lrt

all:
	$(CC) -g -o $(TARGET) $(TARGET).c -Wall -Werror $(TARGET_LDFLAGS)

clean:
	rm -f $(TARGET) *.o
