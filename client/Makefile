CC = gcc
CFLAGS = -Wall -Wextra
LIBS = -lpthread

all: client

client: client.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

clean:
	rm -f client