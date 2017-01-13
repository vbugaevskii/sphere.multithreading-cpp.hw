#include "Server.h"

Server::Server(uint16_t port)
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

    struct epoll_event event;
    event.data.fd = master_socket;
    event.events = EPOLLIN | EPOLLET;

    master_fd = epoll_create1(0);

    if (master_fd == -1)
        throw std::system_error(errno, std::system_category());

    if (epoll_ctl(master_fd , EPOLL_CTL_ADD, master_socket, &event) == -1)
        throw std::system_error(errno, std::system_category());

    events = static_cast<struct epoll_event *>(calloc(MAX_EVENTS, sizeof(struct epoll_event)));

    if (events == NULL)
        throw "Unable to create pull of events.";
}

Server::~Server()
{
    for (auto client: slave_sockets)
        disconnect_client(client);

    struct epoll_event event;
    event.data.fd = master_socket;
    event.events = EPOLLIN | EPOLLET;

    if (epoll_ctl(master_fd , EPOLL_CTL_DEL, master_socket, &event) == -1)
        throw std::system_error(errno, std::system_category());

    free(events);

    close(master_fd);
    shutdown(master_socket, SHUT_RDWR);
    close(master_socket);
}

void Server::start()
{
    while (1) {
        int n_events = epoll_wait(master_fd , events, MAX_EVENTS, 50);

        if (n_events == -1)
            throw std::system_error(errno, std::system_category());

        for (int i = 0; i < n_events; i++)
        {
            if (events[i].data.fd == master_socket)
                process_master_event(events[i]);
            else
                process_client_event(events[i]);
        }
    }
}

void Server::process_master_event(struct epoll_event &event)
{
    do
    {
        int slave_socket = accept(master_socket, 0, 0);
        if (slave_socket > 0)
        {
            struct epoll_event Event;
            Event.data.fd = slave_socket;
            Event.events = EPOLLIN | EPOLLET;

            set_nonblock(slave_socket);

            if (epoll_ctl(master_fd, EPOLL_CTL_ADD, slave_socket, &Event) == -1)
                throw std::system_error(errno, std::system_category());

            messages[slave_socket] = Message();
            slave_sockets.insert(slave_socket);

            std::cout << "[LOG]: accepted connection. " << slave_socket << " joined chat." << std::endl;

            send(slave_socket, "Welcome!\n", 9, MSG_NOSIGNAL);
        }
    }
    while (errno != EAGAIN);
}

void Server::process_client_event(struct epoll_event &event)
{
    int client_fd = event.data.fd;

    if (recv_message(client_fd))
    {
        auto& msg = messages[client_fd];
        if (msg.is_complete())
        {
            for (auto client : slave_sockets)
                send_message(msg, client);

            std::cout << "[MSG from " << client_fd << "]: " << msg;
            msg.clear();
        }

        if (event.events & EPOLLHUP)
            disconnect_client(client_fd);
    }
}

bool Server::recv_message(int client_fd)
{
    static char buffer[BUFF_SIZE];

    do
    {
        ssize_t recv_size = recv(client_fd, buffer, BUFF_SIZE, MSG_NOSIGNAL);

        if (recv_size > 0)
        {
            messages[client_fd].append(buffer, static_cast<size_t>(recv_size));
            if (buffer[recv_size-1] == '\n')
                break;
        }
        else if (recv_size == 0)
        {
            disconnect_client(client_fd);
            return false;
        }
    }
    while (errno != EAGAIN);

    return true;
}

void Server::disconnect_client(int client_fd)
{
    if (epoll_ctl(master_fd , EPOLL_CTL_DEL, client_fd, NULL) == -1)
        throw std::system_error(errno, std::system_category());

    messages.erase(client_fd);
    slave_sockets.erase(client_fd);

    shutdown(client_fd, SHUT_RDWR);
    close(client_fd);

    std::cout << "[LOG]: connection terminated. " << client_fd << std::endl;
}

void Server::send_message(const Message& msg, int client_fd)
{
    size_t msg_size = msg.size();
    for (size_t i = 0; i < msg_size; i++)
        send(client_fd, msg.get_block(i), msg.get_block_size(i), MSG_NOSIGNAL);
}

int set_nonblock(int fd)
{
    int flags;
#if defined(O_NONBLOCK)
    if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
        flags = 0;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
    flags = 1;
	return ioctl(fd, FIOBIO, &flags);
#endif
}
