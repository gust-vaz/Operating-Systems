CC = gcc
CFLAGS = -Wall
LDFLAGS = -lreadline
TARGETS = ep3

.PHONY: clean

all: ${TARGETS}

ep3: ep3.c ep3.h
	${CC} ${CFLAGS} -o ep3 ep3.c ${LDFLAGS}

clean:
	@rm -rf ${TARGETS}

run: ep3
	./ep3