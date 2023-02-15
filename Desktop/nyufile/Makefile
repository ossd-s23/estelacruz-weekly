CC=gcc
LDFLAGS= -l crypto
CFLAGS=-g -Wall -O 

.PHONY: all
all: nyufile
nyufile: nyufile.o
nyufile.o: nyufile.c 
.PHONY: clean
clean:
	rm -f *.o nyufile