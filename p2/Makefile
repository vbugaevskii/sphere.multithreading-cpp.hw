CC=g++
CFLAGS=-c -O1 -Wall -std=c++11
LDFLAGS=
SOURCES_SRV=main_server.cpp Server.cpp Message.cpp
SOURCES_CLT=main_client.cpp Client.cpp Message.cpp
OBJECTS_SRV=$(SOURCES_SRV:.cpp=.o)
OBJECTS_CLT=$(SOURCES_CLT:.cpp=.o)

all: $(SOURCES_SRV) $(SOURCES_CLT) chatsrv chatclt

chatclt: $(OBJECTS_CLT)
	$(CC) $(LDFLAGS) $^ -o $@

chatsrv: $(OBJECTS_SRV)
	$(CC) $(LDFLAGS) $^ -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o chatclt chatsrv

