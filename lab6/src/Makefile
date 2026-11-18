CC = gcc
CFLAGS = -Wall -Wextra -pthread
OBJ = common.o

all: server client

server: server.c $(OBJ)
	$(CC) $(CFLAGS) -o server server.c $(OBJ)

client: client.c $(OBJ)
	$(CC) $(CFLAGS) -o client client.c $(OBJ)

common.o: common.c common.h
	$(CC) $(CFLAGS) -c common.c

clean:
	rm -f server client common.o
