CC=gcc
OPTS=-g -std=c99 -Werror

all: main.o predictor.o
	$(CC) $(OPTS) -g -o predictor main.o predictor.o -lm

main.o: main.c predictor.h
	$(CC) $(OPTS) -g -c main.c

predictor.o: predictor.h predictor.c
	$(CC) $(OPTS) -g -c predictor.c

clean:
	rm -f *.o predictor;
