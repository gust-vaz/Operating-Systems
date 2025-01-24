CC = gcc
CFLAGS = -Wall
LIBS_ep2 = -lpthread

.PHONY: all clean

all: ep2

ep2: ep2.c ep2.h
	$(CC) $(CFLAGS) -o ep2 ep2.h ep2.c $(LIBS_ep2)

clean:
	rm -f ep2