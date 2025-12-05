CC=gcc
CFLAGS=-g -Wall
LIBS=-lncurses -lzmq -lpthread
TARGET=lizard

all: $(TARGET)

$(TARGET): lizard-client.o
	$(CC) $(CFLAGS) -o $(TARGET) lizard-client.o $(LIBS)

lizard-client.o: lizard-client.c
	$(CC) $(CFLAGS) -c lizard-client.c

clean:
	rm -f *.o $(TARGET)
