CC=gcc
CFLAGS=-g -Wall
LIBS=-lncurses -lzmq -lpthread
TARGET=roach

all: $(TARGET)

$(TARGET): roaches-client.o
	$(CC) $(CFLAGS) -o $(TARGET) roaches-client.o $(LIBS)

roaches-client.o: roaches-client.c
	$(CC) $(CFLAGS) -c roaches-client.c

clean:
	rm -f *.o $(TARGET)
