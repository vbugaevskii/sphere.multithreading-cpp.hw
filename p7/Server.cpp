#include "Server.h"

Server::Server(uint16_t port, char * file_name) : f_name(file_name)
{
    p_dict = new Trie(file_name);

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

    delete p_dict;
}

void Server::start()
{
    while (1) {
        struct stat attrib;
        stat(f_name, &attrib);

        if (attrib.st_mtime > time(NULL))
        {
            delete p_dict;
            p_dict = new Trie(f_name);
        }

        int n_events = epoll_wait(master_fd , events, MAX_EVENTS, -1);

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

            slave_sockets.insert(slave_socket);

            std::cout << "[LOG]: " << slave_socket << " connected" << std::endl;

            send(slave_socket, "Welcome!\n", 9, MSG_NOSIGNAL);
        }
    }
    while (errno != EAGAIN);
}

void proccess_message_from_client(Trie * p_dict, char ** words, int * n, char * buffer, char *er)
{
    try
    {
        char buffer_[256];

        if (sscanf(buffer, "%s", buffer_) > 0)
        {
            *er = 0;
            *n = p_dict->get_words(buffer_, words);
            sprintf(buffer, "OK\n");
        }
        else
            *er = -1;
    }
    catch (TrieError err)
    {
        sprintf(buffer, "FAIL: %s\n", err.what());
        *er = 1;
    }
}

void Server::process_client_event(struct epoll_event &event)
{
    int client_fd = event.data.fd;

    if (recv_message_from_client(client_fd))
    {
        char *words[Trie::NUM_WORDS], err;
        int n;

        std::thread thr(proccess_message_from_client, p_dict, words, &n, buffer, &err);
        thr.join();

        if (err == 0)
        {
            send_message_to_client(client_fd);

            for (int i = 0; i < n; i++)
            {
                sprintf(buffer, "OK: %s\n", words[i]);
                send_message_to_client(client_fd);
                free(words[i]);
                words[i] = NULL;
            }

            std::cout << "[LOG]: Command from " << client_fd << " is DONE" << std::endl;
        }
        else if (err == 1)
        {
            std::cout << "[LOG]: Command from " << client_fd << " is FAILED: " << &buffer[6];
            send_message_to_client(client_fd);
        }

        if (event.events & EPOLLHUP)
            disconnect_client(client_fd);
    }
}

bool Server::recv_message_from_client(int client_fd)
{
    ssize_t recv_size = recv(client_fd, buffer, MAX_BUF_SIZE, MSG_NOSIGNAL);

    if (recv_size > 0)
    {
        buffer[recv_size] = 0;
        return true;
    }
    else if (recv_size == 0)
    {
        disconnect_client(client_fd);
        return false;
    }
    else
    {
        throw std::system_error(errno, std::system_category());
    }
}

void Server::send_message_to_client(int client_fd)
{
    send(client_fd, buffer, strlen(buffer) + 1, MSG_NOSIGNAL);
}

void Server::disconnect_client(int client_fd)
{
    if (epoll_ctl(master_fd , EPOLL_CTL_DEL, client_fd, NULL) == -1)
        throw std::system_error(errno, std::system_category());

    slave_sockets.erase(client_fd);

    shutdown(client_fd, SHUT_RDWR);
    close(client_fd);

    std::cout << "[LOG]: " << client_fd << " disconnected" << std::endl;
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