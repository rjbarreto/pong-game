CC = gcc
CFLAGS = -Wall -Wextra -g

all: server client

server:
	$(CC) $(CFLAGS) -o server server.c

client:
	$(CC) $(CFLAGS) -o client client.c -lncurses

clean:
	rm -rf *o client server