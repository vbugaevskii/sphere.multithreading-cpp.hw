#ifndef P2_SERVER_H
#define P2_SERVER_H

#include <iostream>
#include <algorithm>
#include <set>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/epoll.h>

#include <errno.h>
#include <string.h>

#include <wait.h>

#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>

#include "Message.h"
#include "HashTable.h"

class Server
{
public:
    Server(uint16_t port)
    {
        init_worker_resources();
        init_client_resources(port);
    }

    void start();

    ~Server()
    {
        free_client_resources();
        free_worker_resources();
    }
private:
    void init_worker_resources();
    void init_client_resources(uint16_t port);

    void free_worker_resources();
    void free_client_resources();

    void process_master_event(struct epoll_event& event);
    void process_client_event(struct epoll_event& event);

    bool recv_message_from_client(int client_fd);
    void send_message_to_client(int client_fd);

    void disconnect_client(int client_fd);

    char process_message_from_client(Message *msg, int client_fd);
    void process_message_from_worker(Message *msg);

    bool recv_message_from_worker(Message *msg);
    void send_message_to_worker(Message *msg);

public:
    static const size_t MAX_EVENTS = 256;
    static const size_t MAX_BUF_SIZE = 1024;
    static const size_t NUM_WORKERS = 4;

    static const long SERVER_MSGID = 1;

private:
    int master_socket;
    int master_fd;
    struct epoll_event * events;

    std::set<int> slave_sockets;

    char buffer[MAX_BUF_SIZE+1];

    int msgid, shmid, semid;
};

int set_nonblock(int fd);

#endif //P2_SERVER_H
