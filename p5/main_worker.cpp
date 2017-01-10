#include <iostream>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#include "HashTable.h"
#include "Message.h"
#include "Server.h"

static const long SERVER_MSGID = 1;

Message msg;
struct sembuf sops;

int worker_id;
int msgid, shmid, semid;
void *shmaddr;

void sem_up(uint16_t i, short c=1)
{
    sops.sem_num = i;
    sops.sem_flg = 0;
    sops.sem_op =  c;
    semop(semid, &sops, 1);
}

void sem_down(uint16_t i, short c=1)
{
    sem_up(i, -c);
}

int main(int argc, char *argv[])
{
    worker_id = atoi(argv[1]);

    key_t key = ftok("./message", 'b');
    msgid = msgget(key, 0666);

    shmid = shmget(key, HashTable::TOTAL_MEM_SIZE, 0666);
    shmaddr = shmat(shmid, NULL, 0);

    semid = semget(key, HashTable::NUM_SECTIONS, 0666);

    HashTable hash_table(shmaddr);

    while (msgrcv(msgid, (struct msgbuf*) &msg, sizeof(msg), worker_id, 0) > 0)
    {
        msg.mtype = SERVER_MSGID;

        uint16_t sem_id = static_cast<uint16_t>(
                hash_table.get_hash(msg.key) / HashTable::NUM_SECTIONS);

        try
        {
            switch (msg.op)
            {
                case SET:
                    sem_down(sem_id, Server::NUM_WORKERS);
                    hash_table.set(msg.key, msg.value);
                    sem_up(sem_id, Server::NUM_WORKERS);
                    break;

                case GET:
                    sem_down(sem_id);
                    msg.value = hash_table.get(msg.key);
                    sem_up(sem_id);
                    break;

                case DEL:
                    sem_down(sem_id, Server::NUM_WORKERS);
                    hash_table.del(msg.key);
                    sem_up(sem_id, Server::NUM_WORKERS);
                    break;

                case QUIT:
                    shmdt(shmaddr);
                    return 0;

                default: ;
            }

            msg.err = 0;
        }
        catch (HashTableError err)
        {
            switch (err.getType())
            {
                case HashTableErrorType::NoKey:     msg.err = 1; break;
                case HashTableErrorType::NoMemory:  msg.err = 2; break;
            }

            switch (msg.op)
            {
                case SET:
                case DEL:
                    sem_up(sem_id, Server::NUM_WORKERS);
                    break;
                case GET:
                    sem_up(sem_id);
                    break;
                default: ;
            }
        }

        msgsnd(msgid, (struct msgbuf*) &msg, sizeof(msg), 0);
    }

    return 0;
}
