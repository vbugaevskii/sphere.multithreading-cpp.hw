#include "Server.h"

void Server::init_worker_resources()
{
    key_t key = ftok("./message", 'b');
    msgid = msgget(key, 0666 | IPC_CREAT);
    shmid = shmget(key, HashTable::TOTAL_MEM_SIZE, 0666 | IPC_CREAT);
    semid = semget(key, HashTable::NUM_SECTIONS, 0666 | IPC_CREAT);

    void *shmaddr = shmat(shmid, NULL, 0);
    HashTable::shared_memory_set(shmaddr);
    shmdt(shmaddr);

    for (int i = 0; i < NUM_WORKERS; i++)
    {
        if (fork() == 0)
        {
            sprintf(buffer, "%d", i+2);
            execlp("./worker", "./worker", buffer, 0);
            std::cout << "FAILED to create worker! :(" << std::endl;
            exit(1);
        }
    }
}

void Server::init_client_resources(uint16_t port)
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

void Server::free_worker_resources()
{
    Message msg;
    msg.op = QUIT;
    for (int i = 0; i < NUM_WORKERS; i++)
    {
        msg.mtype = i + 2;
        send_message_to_worker(&msg);
    }

    while (wait(NULL) > 0);

    semctl(semid, 0, IPC_RMID, 0);
    shmctl(shmid, IPC_RMID, 0);
    msgctl(msgid, IPC_RMID, 0);
}

void Server::free_client_resources()
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
    Message msg;

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

        if (recv_message_from_worker(&msg))
        {
            process_message_from_worker(&msg);
            send_message_to_client(msg.from);
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

            std::cout << "[LOG]: " << slave_socket  << " connected" << std::endl;

            send(slave_socket, "Welcome!\n", 9, MSG_NOSIGNAL);
        }
    }
    while (errno != EAGAIN);
}

void Server::process_client_event(struct epoll_event &event)
{
    int client_fd = event.data.fd;

    if (recv_message_from_client(client_fd))
    {
        Message msg;
        int res = process_message_from_client(&msg, client_fd);

        if (res > 0)
            send_message_to_worker(&msg);
        else if (res < 0)
            send_message_to_client(client_fd);

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
    }
    else if (recv_size == 0)
    {
        disconnect_client(client_fd);
        return false;
    }

    return true;
}

void Server::disconnect_client(int client_fd)
{
    if (epoll_ctl(master_fd , EPOLL_CTL_DEL, client_fd, NULL) == -1)
        throw std::system_error(errno, std::system_category());

    slave_sockets.erase(client_fd);

    shutdown(client_fd, SHUT_RDWR);
    close(client_fd);

    std::cout << "[LOG]: " << client_fd  << " disconnected" << std::endl;
}

void Server::send_message_to_client(int client_fd)
{
    send(client_fd, buffer, strlen(buffer) + 1, MSG_NOSIGNAL);
}

char Server::process_message_from_client(Message *msg, int client_fd)
{
    char cmd[32];
    int key, value, n_args;

    if ((n_args = sscanf(buffer, "%s%d%d", cmd, &key, &value)) <= 0)
        return 0;

    std::cout << "[CMD from " << client_fd << "]:";

    msg->key   = key;
    msg->value = value;
    msg->from  = client_fd;
    msg->mtype = int(rand() % NUM_WORKERS) + 2;

    if (strcmp(cmd, "set") == 0 && n_args == 3)
    {
        msg->op = SET;
    }
    else if (strcmp(cmd, "get") == 0 && n_args == 2)
    {
        msg->op = GET;
    }
    else if (strcmp(cmd, "del") == 0 && n_args == 2)
    {
        msg->op = DEL;
    }
    else
    {
        std::cout << buffer;

        std::cout << "[LOG]: Command from " << msg->from << " is FAILED: "
                  << "Wrong command or wrong number of params" << std::endl;

        sprintf(buffer, "> FAIL: Wrong command or wrong number of params\n");
        return -1;
    }

    if (n_args >= 1)
    {
        std::cout << ' ' << cmd;
        if (n_args >= 2)
        {
            std::cout << ' ' << key;
            if (n_args >= 3)
                std::cout << ' ' << value;
        }
    }
    std::cout << std::endl;
    return 1;
}

void Server::process_message_from_worker(Message *msg)
{
    switch (msg->err)
    {
        case 0:
            std::cout << "[LOG]: Command from " << msg->from << " is DONE" << std::endl;
            sprintf(buffer, (msg->op == GET) ? "> DONE: (%d , %d)\n" : "> DONE\n", msg->key, msg->value);
            break;

        case 1:
            std::cout << "[LOG]: Command from " << msg->from << " is FAILED: "
                      << "There is no key " << msg->key << std::endl;
            sprintf(buffer, "> FAIL: There is no key %d\n", msg->key);
            break;

        case 2:
            std::cout << "[LOG]: Command from " << msg->from << " is FAILED: "
                      << "There is no more free memory left" << std::endl;
            sprintf(buffer, "> FAIL: There is no more free memory left\n");
            break;

        default: ;
    }
}

void Server::send_message_to_worker(Message *msg)
{
    msgsnd(msgid, (struct msgbuf*) msg, sizeof(Message), 0);
}

bool Server::recv_message_from_worker(Message *msg)
{
    return (msgrcv(msgid, (struct msgbuf*) msg, sizeof(Message), SERVER_MSGID, IPC_NOWAIT) > 0);
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