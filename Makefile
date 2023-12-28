CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -pedantic -Wno-unused-result -g -O0 -D_XOPEN_SOURCE -D_DEFAULT_SOURCE

.PHONY = all clean

all: dim

clean:
	@rm -f *.o *.gch *~

dim: dim.o
	$(CC) dim.o -o dim

dim.o: dim.c termbox2.h
