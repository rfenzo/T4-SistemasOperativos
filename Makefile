CC = gcc
CFLAGS = -g -Wall

all: server client

server: server.o
	$(CC) $(CFLAGS) -o server  server.o -lm

server.o: server.c
	$(CC) $(CFLAGS) -c server.c -lm

client: client.o
	$(CC) $(CFLAGS) -o client  client.o -lm

client.o: client.c
	$(CC) $(CFLAGS) -c client.c -lm

clean:
	$(RM) server client *.o *~
