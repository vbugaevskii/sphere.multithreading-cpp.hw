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

#define MAX_EVENTS 256

#include "Message.h"

class Server
{
public:
    Server(uint16_t port);

    void start();

    ~Server();
private:
    void process_master_event(struct epoll_event& event);
    void process_client_event(struct epoll_event& event);

    bool recv_message(int client_fd);
    void send_message(const Message& msg, int client_fd);

    void disconnect_client(int client_fd);

private:
    int master_socket;
    int master_fd;
    struct epoll_event * events;

    std::map<int, Message> messages;
    std::set<int> slave_sockets;
};

int set_nonblock(int fd);

#endif //P2_SERVER_H
