CC=gcc
CFLAGS=-Wall -O3

DEPS =

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: build

build: ffws.o
	$(CC) -o ffws ffws.o

clean:
	rm -f *.o ffws
