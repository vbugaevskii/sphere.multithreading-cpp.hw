#ifndef P5_1_ROUTINE_H
#define P5_1_ROUTINE_H

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

static const size_t MAX_EVENTS = 256;
static const size_t MAX_BUF_SIZE = 1024;

static const size_t NUM_WORKERS = 4;

int set_nonblock(int fd);

ssize_t sock_fd_write(int sock, void *buf, ssize_t buflen, int fd);
ssize_t sock_fd_read(int sock, void *buf, ssize_t bufsize, int *fd);

#endif //P5_1_ROUTINE_H
