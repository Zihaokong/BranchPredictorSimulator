CC=gcc
OPTS=-g -std=c99 -Werror

all: main.o cache.o
	$(CC) $(OPTS) -lm -o cache main.o cache.o -g

main.o: main.c cache.h
	$(CC) $(OPTS) -c main.c -g

cache.o: cache.h cache.c
	$(CC) $(OPTS) -c cache.c -g

clean:
	rm -f *.o cache;
