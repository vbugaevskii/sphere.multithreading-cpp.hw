#include "Operand.h"

extern std::set<pid_t> slave_pids;

Command::Command(const std::vector<std::string>& cmd)
{
    is_input_modified = is_output_modified = false;

    args = (char **) malloc((cmd.size() + 1) * sizeof(char *));
    int j = 0;

    for (int i = 0; i < cmd.size(); i++)
    {
        if (cmd[i] == "<")
        {
            fd_input = open(cmd[++i].c_str(), O_RDONLY);
            is_input_modified = true;
        }
        else if (cmd[i] == ">")
        {
            fd_output = open(cmd[++i].c_str(), O_WRONLY | O_CREAT | O_APPEND | O_TRUNC, 0666);
            is_output_modified = true;
        }
        else
        {
            args[j] = (char *) calloc(cmd[i].size() + 1, sizeof(char));
            memcpy(args[j++], cmd[i].c_str(), cmd[i].size() + 1);
        }
    }

    args[j] = NULL;
    n_args = j + 1;
    args = (char **) realloc(args, n_args * sizeof(char *));
}

void Command::run()
{
    pid_t pid;
    if ((pid = fork()) == 0)
    {
        if (is_input_modified)
        {
            dup2(fd_input, STDIN_FILENO);
            close(fd_input);
        }

        if (is_output_modified)
        {
            dup2(fd_output, STDOUT_FILENO);
            close(fd_output);
        }

        execvp(args[0], args);
    }
    else
    {
        slave_pids.insert(pid);

        if (is_input_modified)
            close(fd_input);

        if (is_output_modified)
            close(fd_output);

        pid_t pid = wait(&status);
        int code = WIFEXITED(status) ? WEXITSTATUS(status) : WIFEXITED(status);
        std::cerr << "Process " << pid << " exited: " << code << std::endl;

        slave_pids.erase(pid);
    }
}

void Command::set_fd_input(int fd)
{
    fd_input = fd;
    is_input_modified = true;
}

void Command::set_fd_output(int fd)
{
    fd_output = fd;
    is_output_modified = true;
}

Command::~Command()
{
    for (int i = 0; i < n_args - 1; i++)
        free(args[i]);
    free(args);
}
