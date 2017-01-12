#ifndef P2_SERVER_H
#define P2_SERVER_H

#include <iostream>
#include <algorithm>
#include <map>
#include <set>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/epoll.h>

#include <errno.h>
#include <string.h>

#include <thread>
#include <sys/stat.h>

#include "Trie.h"

class Server
{
public:
    Server(uint16_t port, char * file_name);

    void start();

    ~Server();
private:
    void process_master_event(struct epoll_event& event);
    void process_client_event(struct epoll_event& event);

    bool recv_message_from_client(int client_fd);
    void send_message_to_client(int client_fd);

    void disconnect_client(int client_fd);

public:
    static const size_t MAX_EVENTS = 256;
    static const size_t MAX_BUF_SIZE = 1024;

private:
    char buffer[MAX_BUF_SIZE];

    int master_socket;
    int master_fd;
    struct epoll_event * events;

    std::set<int> slave_sockets;

    Trie * p_dict;

    char * f_name;
};

int set_nonblock(int fd);

void proccess_message_from_client(Trie * p_dict, char ** words, int * n, char * buffer, char * err);

#endif //P2_SERVER_H
