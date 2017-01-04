CC=g++
CFLAGS=-std=c++11
LDFLAGS=-std=c++11
SOURCES=main.cpp Parser.cpp Parser.h Operand.cpp Operand.h

all: main

main: $(SOURCES)
	$(CC) $(LDFLAGS) -o main $(SOURCES)

clean:
	rm -f *.o main file* ./tests/*.run.*

