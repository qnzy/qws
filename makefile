CC=gcc
CFLAGS=-Wall -O3

DEPS =

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: build

build: qws.o
	$(CC) -o qws qws.o

clean:
	rm -f *.o qws
