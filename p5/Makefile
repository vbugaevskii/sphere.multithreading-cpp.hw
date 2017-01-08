CC=g++
CFLAGS=-std=c++11
LDFLAGS=-std=c++11
SOURCES_SERVER=main_server.cpp Server.cpp Server.h Message.h
SOURCES_WORKER=main_worker.cpp HashTable.cpp HashTable.h Message.h

all: server worker

server: $(SOURCES_SERVER)
	$(CC) $(LDFLAGS) $(SOURCES_SERVER) -o server

worker: $(SOURCES_WORKER)
	$(CC) $(LDFLAGS) $(SOURCES_WORKER) -o worker

clean:
	rm -f *.o worker server

