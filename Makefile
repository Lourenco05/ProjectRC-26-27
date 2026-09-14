CC = gcc
CFLAGS = -Wall -Wextra -pedantic

all: user

user: user.c
	$(CC) $(CFLAGS) user.c -o user

clean:
	rm -f user