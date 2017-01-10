#include "Server.h"

Server::Server(uint16_t port, int *fd_passing) : fd_passing_writers(fd_passing)
{
    master_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP | SO_REUSEADDR);

    if (master_socket == -1)
        throw std::system_error(errno, std::system_category());

    struct sockaddr_in socket_address;
    socket_address.sin_family = AF_INET;
    socket_address.sin_port = htons(port);
    socket_address.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(master_socket, (struct sockaddr *)&socket_address, sizeof(socket_address)) == -1)
        throw std::system_error(errno, std::system_category());

    set_nonblock(master_socket);

    if (listen(master_socket, SOMAXCONN) == -1)
        throw std::system_error(errno, std::system_category());

    master_fd = epoll_create1(0);

    if (master_fd == -1)
        throw std::system_error(errno, std::system_category());

    struct epoll_event event;
    event.data.fd = master_socket;
    event.events = EPOLLIN | EPOLLET;

    if (epoll_ctl(master_fd , EPOLL_CTL_ADD, master_socket, &event) == -1)
        throw std::system_error(errno, std::system_category());

    events = static_cast<struct epoll_event *>(calloc(MAX_EVENTS, sizeof(struct epoll_event)));

    if (events == NULL)
        throw "Unable to create pull of events.";
}

void Server::start()
{
    while (1)
    {
        int n_events = epoll_wait(master_fd , events, MAX_EVENTS, 50);

        if (n_events == -1)
            throw std::system_error(errno, std::system_category());

        for (int i = 0; i < n_events; i++)
        {
            if (events[i].data.fd == master_socket)
                process_master_event(events[i]);
        }
    }
}

Server::~Server()
{
    struct epoll_event event;
    event.data.fd = master_socket;
    event.events = EPOLLIN | EPOLLET;

    if (epoll_ctl(master_fd , EPOLL_CTL_DEL, master_socket, &event) == -1)
        throw std::system_error(errno, std::system_category());

    free(events);

    close(master_fd);
    shutdown(master_socket, SHUT_RDWR);
    close(master_socket);

    for (int i = 0; i < NUM_WORKERS; i++)
        close(fd_passing_writers[i]);
}

void Server::process_master_event(struct epoll_event &event_)
{
    int slave_socket = accept(master_socket, 0, 0);

    if (slave_socket > 0)
    {
        set_nonblock(slave_socket);
        sock_fd_write(fd_passing_writers[rand() % NUM_WORKERS], const_cast<char *>("1"), 1, slave_socket);
    }
}

