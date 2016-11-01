CC=g++
CFLAGS=-std=c++11
LDFLAGS=-std=c++11
SOURCES=main_proxy.cpp proxy.cpp proxy.h config_parser.cpp config_parser.h

all: proxy

proxy: $(SOURCES)
	$(CC) $(LDFLAGS) $(SOURCES) -o proxy -lboost_system

clean:
	rm -f *.o proxy

