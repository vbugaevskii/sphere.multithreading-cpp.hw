#include "Message.h"

Message::Message(const std::string& msg)
{
    const char *message = msg.c_str();

    for (size_t i = 0; i < msg.size() + 1; i += BUFF_SIZE)
        append(message + i, std::min(strlen(message + i), static_cast<size_t>(BUFF_SIZE)));
}

Message::~Message()
{
    for (auto msg: blocks) free(msg);
}

void Message::append(const char *msg, size_t size)
{
    char *part = (char *) calloc(size, 1);
    memcpy(part, msg, size);
    blocks.push_back(part);
    blocks_size.push_back(size);
}

void Message::clear()
{
    blocks.clear();
    blocks_size.clear();
}

std::ostream& operator<< (std::ostream& out, const Message& msg)
{
    std::string message = "";
    for (size_t i = 0; i < msg.blocks.size(); i++)
        message.append(msg.blocks[i], msg.blocks_size[i]);
    out << message;
    out.flush();
    return out;
}

Message& Message::operator+= (const Message& msg)
{
    blocks.insert(blocks.end(), msg.blocks.begin(), msg.blocks.end());
    blocks_size.insert(blocks_size.end(), msg.blocks_size.begin(), msg.blocks_size.end());
    return *this;
}