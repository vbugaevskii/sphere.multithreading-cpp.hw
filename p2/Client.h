#ifndef P2_CLIENT_H
#define P2_CLIENT_H

#include <iostream>
#include <algorithm>
#include <set>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/select.h>

#include <errno.h>
#include <string.h>

#include "Message.h"

class Client
{
public:
    Client(uint16_t port);

    void start();

    ~Client();
private:
    void send_message();
    void recv_message_from_srv();
    void recv_message_from_stdin();

private:
    int server_fd;
    fd_set select_fd[2];

    Message messages[2];
};

int set_nonblock(int fd);

#endif //P2_CLIENT_H
