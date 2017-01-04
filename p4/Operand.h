#ifndef P4_OPERAND_H
#define P4_OPERAND_H

#include <iostream>

#include <sys/types.h>
#include <unistd.h>
#include <wait.h>

#include <cstring>

#include <string>
#include <vector>
#include <set>

#include <fcntl.h>


class Operand
{
public:
    virtual const std::string type() const = 0;
};


class Command : public Operand
{
    char **args;

    int status, n_args;

    int fd_input, fd_output;
    bool is_input_modified, is_output_modified;

public:
    Command(const std::vector<std::string>& cmd);

    void run();

    void set_fd_input(int fd);
    void set_fd_output(int fd);

    const std::string type() const { return "cmd"; }
    int get_exit_status() const { return status; }

    ~Command();
};


typedef enum
{
    LOGICAL_AND,
    LOGICAL_OR,
    CONVEYOR,
    BACKGROUND_MODE,
    SEPARATOR
} op_type;


class Operator : public Operand
{
public:
    Operator(op_type o) : op_name(o) {}

    const std::string type() const { return "op"; }
    const op_type name() { return  op_name; }

private:
    op_type op_name;
};

#endif //P4_OPERAND_H
