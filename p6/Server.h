#ifndef P2_SERVER_H
#define P2_SERVER_H

#include "Routine.h"

class Server
{
public:
    Server(uint16_t port, int *fd_passing);

    void start();

    ~Server();

private:
    void process_master_event(struct epoll_event &event_);

public:
    static const size_t NUM_WORKERS = 4;

private:
    int master_socket;
    int master_fd;
    int *fd_passing_writers;

    struct epoll_event * events;
};

#endif //P2_SERVER_H
