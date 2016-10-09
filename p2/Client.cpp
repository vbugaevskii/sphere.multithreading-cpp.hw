#include "Client.h"

Client::Client(uint16_t port)
{
    server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (server_fd == -1)
        throw std::system_error(errno, std::system_category());

    struct sockaddr_in socket_address;
    socket_address.sin_family = AF_INET;
    socket_address.sin_port = htons(port);
    socket_address.sin_addr.s_addr = htonl(INADDR_ANY);

    if (connect(server_fd, (struct sockaddr *)&socket_address, sizeof(socket_address)) == -1 && errno != EINPROGRESS)
        throw std::system_error(errno, std::system_category());
}

Client::~Client()
{
    shutdown(server_fd, SHUT_RDWR);
    close(server_fd);
}

void Client::start()
{
    while (1)
    {
        FD_ZERO(&select_fd[0]);
        FD_SET(server_fd, &select_fd[0]);
        FD_SET(0, &select_fd[0]);

        FD_ZERO(&select_fd[1]);
        FD_SET(server_fd, &select_fd[1]);

        if (select(std::max(0, server_fd) + 1, &select_fd[0], &select_fd[1], NULL, NULL) == -1)
            throw std::system_error(errno, std::system_category());

        if (FD_ISSET(0, &select_fd[0]))
        {
            recv_message_from_stdin();
            send_message();
        }

        if (FD_ISSET(server_fd, &select_fd[0]))
        {
            recv_message_from_srv();
            std::cout << messages[1];
            messages[1].clear();
        }
    }
}

void Client::recv_message_from_stdin()
{
    static char buffer[BUFF_SIZE + 1];

    do
    {
        if (fgets(buffer, BUFF_SIZE + 1, stdin) == NULL)
            break;

        std::cout << "\x1b[1A" << "\x1b[2K";

        size_t msg_size = strlen(buffer);
        if (msg_size == 1 && buffer[0] == '\n')
            break;

        messages[0].append(buffer, msg_size);
        if (buffer[msg_size - 1] == '\n')
            break;
    }
    while (1);
}

void Client::recv_message_from_srv()
{
    static char buffer[BUFF_SIZE];

    do
    {
        ssize_t recv_size = recv(server_fd, buffer, BUFF_SIZE, MSG_NOSIGNAL);

        if (recv_size > 0)
        {
            messages[1].append(buffer, static_cast<size_t>(recv_size));
            if (buffer[recv_size-1] == '\n')
                break;
        }
        else if (recv_size == 0)
            throw "Server is not answering...";
    }
    while (errno != EAGAIN);
}

void Client::send_message()
{
    Message& msg = messages[0];
    if (FD_ISSET(server_fd, &select_fd[1]) && msg.size())
    {
        size_t msg_size = msg.size();
        for (size_t i = 0; i < msg_size; i++)
            send(server_fd, msg.get_block(i), msg.get_block_size(i), MSG_NOSIGNAL);
        msg.clear();
    }
}