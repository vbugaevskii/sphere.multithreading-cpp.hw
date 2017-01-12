CC=g++
CFLAGS=-std=c++11 -lrt -pthread

SRC_SERVER=Server.cpp Server.h
SRC_WORKER=Worker.cpp Worker.h
SRC_HASH=HashTable.cpp HashTable.h
SRC_ROUTINE=Routine.cpp Routine.h

all:
	$(CC) main.cpp $(SRC_SERVER) $(SRC_WORKER) $(SRC_HASH) $(SRC_ROUTINE) -o server $(CFLAGS)

clean:
	rm -f *.o server

