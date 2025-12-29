CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lhiredis

all: ovsx

ovsx: ovsx.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f ovsx

run: ovsx
	./ovsx
