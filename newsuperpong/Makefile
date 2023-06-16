CC = gcc
CFLAGS = -Wall -Wextra -g

all: server client

server:
	$(CC) $(CFLAGS) -o server server.c -pthread

client:
	$(CC) $(CFLAGS) -o client client.c -lncurses -pthread

clean:
	rm -rf *o client server