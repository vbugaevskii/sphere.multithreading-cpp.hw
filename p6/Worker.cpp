#include "Worker.h"
#include "Server.h"


Worker::Worker(void *shmaddr, sem_t **sems, int fd_passing) :
        fd_passing_reader(fd_passing), semids(sems), hash_table(shmaddr)
{
    master_fd = epoll_create1(0);

    if (master_fd == -1)
        throw std::system_error(errno, std::system_category());

    struct epoll_event event;
    event.data.fd = fd_passing;
    event.events = EPOLLIN | EPOLLET;

    if (epoll_ctl(master_fd , EPOLL_CTL_ADD, fd_passing, &event) == 50)
        throw std::system_error(errno, std::system_category());

    events = static_cast<struct epoll_event *>(calloc(MAX_EVENTS, sizeof(struct epoll_event)));

    if (events == NULL)
        throw "Unable to create pull of events.";
}

void Worker::start()
{
    while (1)
    {
        int n_events = epoll_wait(master_fd , events, MAX_EVENTS, 50);

        if (n_events == -1)
            throw std::system_error(errno, std::system_category());

        for (int i = 0; i < n_events; i++)
        {
            if (events[i].data.fd == fd_passing_reader)
                process_master_event(events[i]);
            else
                process_client_event(events[i]);
        }
    }
}

Worker::~Worker()
{
    for (auto client: slave_sockets)
        disconnect_client(client);

    struct epoll_event event;
    event.data.fd = fd_passing_reader;
    event.events = EPOLLIN | EPOLLET;

    if (epoll_ctl(master_fd , EPOLL_CTL_DEL, fd_passing_reader, &event) == -1)
        throw std::system_error(errno, std::system_category());

    free(events);
    close(master_fd);

    close(fd_passing_reader);
}

void Worker::process_master_event(struct epoll_event &event_)
{
    char c;
    int slave_socket;

    sock_fd_read(fd_passing_reader, &c, 1, &slave_socket);

    struct epoll_event event;
    event.data.fd = slave_socket;
    event.events = EPOLLIN | EPOLLET;

    if (epoll_ctl(master_fd, EPOLL_CTL_ADD, slave_socket, &event) == -1)
        throw std::system_error(errno, std::system_category());

    slave_sockets.insert(slave_socket);

    std::cout << "[LOG]: accepted connection. " << slave_socket << " joined chat." << std::endl;
    send(slave_socket, "Welcome!\n", 9, MSG_NOSIGNAL);
}

void Worker::process_client_event(struct epoll_event &event)
{
    int client_fd = event.data.fd;

    if (recv_message_from_client(client_fd))
    {
        Query query;
        int res;

        if ((res = process_query(client_fd, &query)) != 0)
        {
            if (res > 0)
                execute_query(query);

            send_message_to_client(client_fd);
        }

        if (event.events & EPOLLHUP)
            disconnect_client(client_fd);
    }
}

char Worker::process_query(int client_fd, Query *p_query)
{
    char cmd[32];
    int key, ttl, value, n_args;

    if ((n_args = sscanf(buffer, "%s%d%d%d", cmd, &key, &ttl, &value)) <= 0)
        return 0;

    p_query->key = key;
    p_query->value = value;
    p_query->from = client_fd;
    p_query->ttl = ttl;

    std::cout << "[CMD from " << client_fd << "]:";

    if (strcmp(cmd, "set") == 0 && n_args == 4)
    {
        p_query->op = SET;
    }
    else if (strcmp(cmd, "get") == 0 && n_args == 2)
    {
        p_query->op = GET;
    }
    else if (strcmp(cmd, "del") == 0 && n_args == 2)
    {
        p_query->op = DEL;
    }
    else
    {
        std::cout << buffer;

        std::cout << "[LOG]: Command from " << client_fd << " is FAILED: "
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
            if (n_args >= 4)
                std::cout << ' ' << ttl << ' ' << value;
        }
    }
    std::cout << std::endl;
    return 1;
}

void Worker::execute_query(Query query)
{
    int sem_id = hash_table.get_hash(query.key) / HashTable::NUM_SECTIONS;

    sem_down(sem_id);

    try
    {
        switch (query.op)
        {
            case SET:
                hash_table.set(query.key, query.ttl, query.value);
                break;

            case GET:
                query.value = hash_table.get(query.key);
                break;

            case DEL:
                hash_table.del(query.key);
                break;

            default: ;
        }

        std::cout << "[LOG]: Command from " << query.from << " is DONE" << std::endl;
        sprintf(buffer, (query.op == GET) ? "> DONE: (%d , %d)\n" : "> DONE\n", query.key, query.value);
    }
    catch (HashTableError err)
    {
        switch (err.getType())
        {
            case HashTableErrorType::NoKey:
                std::cout << "[LOG]: Command from " << query.from << " is FAILED: "
                          << "There is no key " << query.key << std::endl;
                sprintf(buffer, "> FAIL: There is no key %d\n", query.key);
                break;

            case HashTableErrorType::NoMemory:
                std::cout << "[LOG]: Command from " << query.from << " is FAILED: "
                          << "There is no more free memory left" << std::endl;
                sprintf(buffer, "> FAIL: There is no more free memory left\n");
                break;

            case HashTableErrorType::NotExists:
                std::cout << "[LOG]: Command from " << query.from << " is FAILED: "
                          << "Element with key " << query.key << " is expired" << std::endl;
                sprintf(buffer, "> FAIL: Element with key %d is expired\n", query.key);
                break;
        }
    }

    sem_up(sem_id);
}

bool Worker::recv_message_from_client(int client_fd)
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

void Worker::send_message_to_client(int client_fd)
{
    send(client_fd, buffer, strlen(buffer) + 1, MSG_NOSIGNAL);
}

void Worker::disconnect_client(int client_fd)
{
    if (epoll_ctl(master_fd , EPOLL_CTL_DEL, client_fd, NULL) == -1)
        throw std::system_error(errno, std::system_category());

    slave_sockets.erase(client_fd);

    shutdown(client_fd, SHUT_RDWR);
    close(client_fd);

    std::cout << "[LOG]: " << client_fd  << " disconnected" << std::endl;
}

