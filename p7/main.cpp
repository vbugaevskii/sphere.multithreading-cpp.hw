#include <iostream>
#include <cstring>
#include <signal.h>

#include "Server.h"

Server *server = NULL;

void handler(int signum)
{
    if (signum == SIGINT || signum == SIGTERM)
    {
        delete server;
        exit(0);
    }
}

int main(int argc, char *argv[])
{
    signal(SIGINT, &handler);
    signal(SIGTERM, &handler);

    try
    {
        server = new Server(13666, argv[1]);
        server->start();
        delete server;
    }
    catch (const std::system_error& error)
    {
        if (server)
        {
            delete server;
            server = NULL;
        }
        std::cout << "Error: " << error.code()
                  << " - " << error.code().message() << '\n';
    }
    catch (char const * error)
    {
        if (server)
        {
            delete server;
            server = NULL;
        }
        std::cout << error << std::endl;
    }

    return 0;
}