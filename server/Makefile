CC = gcc
CFLAGS = -Wall -Wextra
LIBS = -lsqlite3 -lpthread

all: server

server: server.o database.o auth.o
	$(CC) $(CFLAGS) -o server $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o server