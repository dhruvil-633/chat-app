CC = gcc
CFLAGS = -Wall -Wextra
LIBS = -lsqlite3 -lpthread -lbcrypt

SRC = server.c database.c auth.c
OBJ = $(SRC:.c=.o)
EXEC = server

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJ) $(EXEC)