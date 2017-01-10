#ifndef P5_1_WORKER_H
#define P5_1_WORKER_H

#include <semaphore.h>

#include "Routine.h"
#include "HashTable.h"

enum op_type { GET, SET, DEL };

struct Query
{
    int key, value, from, ttl;
    op_type op;
};

class Worker
{
public:
    Worker(void *shmaddr, sem_t **sems, int fd_passing);

    void start();

    ~Worker();

private:
    void process_master_event(struct epoll_event &event_);
    void process_client_event(struct epoll_event &event);

    char process_query(int client_fd, Query *p_query);
    void execute_query(Query query);

    bool recv_message_from_client(int client_fd);
    void send_message_to_client(int client_fd);

    void disconnect_client(int client_fd);

    void sem_up(int i, int c=1)
    {
        for (int j = 0; j < c; j++)
            sem_wait(semids[i]);
    }

    void sem_down(int i, int c=1)
    {
        for (int j = 0; j < c; j++)
            sem_post(semids[i]);
    }

private:
    int master_fd;
    int fd_passing_reader;

    struct epoll_event * events;
    std::set<int> slave_sockets;

    char buffer[MAX_BUF_SIZE+1];

    HashTable hash_table;
    sem_t **semids;
};

#endif //P5_1_WORKER_H
