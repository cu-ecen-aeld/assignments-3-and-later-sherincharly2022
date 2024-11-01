TARGET=aesdsocket

CC ?= gcc 
TARGET_LDFLAGS ?= -pthread -lrt

all:
	$(CC) $(CLAGS) -o $(TARGET) $(TARGET).c $(TARGET_LDFLAGS)

clean:
	rm -f $(TARGET) *.o
