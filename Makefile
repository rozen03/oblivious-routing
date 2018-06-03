.PHONY : all clean run new

MPIROOT = /usr

CFLAGS+= -Wall -std=c++11 -pthread -g $(INCL)
CC=gcc
MPICC=  $(MPIROOT)/bin/mpic++
INCL= -I$(MPIROOT)/include
SRCS= node.cpp block.cpp main.cpp
BIN= main


all: main

$(BIN): $(SRCS)
	$(MPICC) $(CFLAGS) -o $(BIN) $(SRCS)

clean:
	rm -f $(BIN) *.o

new: clean all

run: main
	mpirun -np 4 ./main
