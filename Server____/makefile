CC=gcc
CFLAGS=-g -Wall
LIBS=-lncurses -lzmq -lpthread
TARGET=server

all: $(TARGET)

$(TARGET): lizardsNroaches-server.o list_handling.o
	$(CC) $(CFLAGS) -o $(TARGET) lizardsNroaches-server.o list_handling.o $(LIBS)

lizardsNroaches-server.o: lizardsNroaches-server.c
	$(CC) $(CFLAGS) -c lizardsNroaches-server.c

list_handling.o: list_handling.c
	$(CC) $(CFLAGS) -c list_handling.c

clean:
	rm -f *.o $(TARGET)