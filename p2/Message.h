#ifndef P2_MESSAGE_H
#define P2_MESSAGE_H

#include <iostream>
#include <algorithm>
#include <vector>

#include <cstdlib>

#include <string.h>

#define BUFF_SIZE 1024

class Message
{
public:
    Message() { }
    Message(const std::string& msg);

    void append(const char *msg, size_t size);
    void clear();

    char * get_block(size_t i) const { return blocks[i]; }
    size_t get_block_size(size_t i) const { return blocks_size[i]; }

    size_t size() const { return blocks.size(); }
    bool is_complete() const {
        char * p = blocks.back();
        size_t n = blocks_size.back();
        return p[n - 1] == '\n';
    }

    friend std::ostream& operator<< (std::ostream& out, const Message& msg);

    Message& operator+= (const Message& msg);

    ~Message();

private:
    std::vector<char *> blocks;
    std::vector<size_t> blocks_size;
};

#endif //P2_MESSAGE_H
