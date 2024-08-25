TARGET=aesdsocket

GCC ?= gcc 

all:
	$(GCC) -g -o $(TARGET) $(TARGET).c -Wall -Werror -lpthread -lrt

clean:
	rm -f $(TARGET) *.o
