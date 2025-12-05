CC=gcc
CFLAGS=-g -Wall
LIBS=-lncurses -lzmq -lpthread
TARGET=wasp

all: $(TARGET)

$(TARGET): wasps-client.o
	$(CC) $(CFLAGS) -o $(TARGET) wasps-client.o $(LIBS)

wasps-client.o: wasps-client.c
	$(CC) $(CFLAGS) -c wasps-client.c

clean:
	rm -f *.o $(TARGET)
