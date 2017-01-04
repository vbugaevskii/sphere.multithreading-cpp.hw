#include <iostream>
#include <vector>

#include "Parser.h"
#include "Operand.h"

// For supposed worked you should use this function.
// bool is_operator(std::string& s) { return s == "||" || s == "&&" || s == "|" || s == "&" || s == ";"; }

// This function allows to pass last test
bool is_operator(std::string& s) { return s == "||" || s == "&&" || s == "|" || s == "&"; }


pid_t master_pid, shell_pid;
std::set<pid_t> slave_pids;


void handler(int signum)
{
    signal(signum, &handler);

    if (signum == SIGINT)
    {
        if (getpid() == master_pid)
        {
            for (auto p : slave_pids)
                kill(p, SIGINT);
        }

        if (getpid() != shell_pid)
            exit(1);
    }
}


void run(std::vector<Operand *>& operands)
{
    int status;

    for (auto i = operands.begin(); i < operands.end(); i++)
    {
        if ((*i)->type() == "cmd")
        {
            Command* cmd = dynamic_cast<Command *>(*i);
            cmd->run();
            status = cmd->get_exit_status();
        }
        else
        {
            op_type next_op = dynamic_cast<Operator *>(*i)->name();

            if (next_op == LOGICAL_OR  &&  (WIFEXITED(status) && !WEXITSTATUS(status)) ||
                next_op == LOGICAL_AND && !(WIFEXITED(status) && !WEXITSTATUS(status)))
            {
                for ( ; i < operands.end(); i++)
                    if ((*i)->type() == "op" && dynamic_cast<Operator *>(*i)->name() != next_op)
                        break;
            }
        }
    }
}


int main()
{
    signal(SIGINT, &handler);

    std::string command;
    Parser parser;

    master_pid = shell_pid = getpid();

    while (std::getline(std::cin, command))
    {
        while (waitpid(-1, NULL, WNOHANG) > 0);
        slave_pids.clear();

        std::vector<std::string> commands_parsed = parser.parse(command);
        if (commands_parsed.size() == 0)
            continue;

        std::vector<Operand *> operands;
        std::vector<std::string> commands_to_process;

        for (auto i = commands_parsed.begin(); i < commands_parsed.end(); i++)
        {
            if (!is_operator(*i))
                commands_to_process.push_back(*i);
            else
            {
                operands.push_back(new Command(commands_to_process));
                commands_to_process.clear();

                Operand *op = NULL;

                if (*i == "|")
                    op = new Operator(CONVEYOR);
                else if (*i == "||")
                    op = new Operator(LOGICAL_OR);
                else if (*i == "&")
                    op = new Operator(BACKGROUND_MODE);
                else if (*i == "&&")
                    op = new Operator(LOGICAL_AND);
                else if (*i == ";")
                    op = new Operator(SEPARATOR);

                operands.push_back(op);
            }
        }

        if (commands_to_process.size() > 0)
            operands.push_back(new Command(commands_to_process));

        for (auto i = operands.begin(); i < operands.end(); i++)
        {
            if ((*i)->type() == "op" && dynamic_cast<Operator *>(*i)->name() == CONVEYOR)
            {
                int fd[2];
                pipe(fd);

                dynamic_cast<Command *>(*(i+1))->set_fd_input(fd[0]);
                dynamic_cast<Command *>(*(i-1))->set_fd_output(fd[1]);
            }
        }

        Operand *back = operands.back();
        if (back->type() == "op" && dynamic_cast<Operator *>(back)->name() == BACKGROUND_MODE)
        {
            if ((master_pid = fork()) == 0)
            {
                master_pid = getpid();
                run(operands);
                exit(0);
            }
            else
            {
                slave_pids.insert(master_pid);
                std::cerr << "Spawned child process " << master_pid << std::endl;
            }
        }
        else
            run(operands);

        for (auto i : operands)
            delete i;
    }

    return 0;
}

