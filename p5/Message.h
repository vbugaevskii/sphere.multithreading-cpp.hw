#ifndef P2_MESSAGE_H
#define P2_MESSAGE_H

#include <sys/ipc.h>
#include <sys/msg.h>

enum op_type { GET, SET, DEL, QUIT };

struct Message
{
    long mtype;
    op_type op;
    int key, value, err, from;
};

#endif //P2_MESSAGE_H
