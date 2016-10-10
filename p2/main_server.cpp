#include <iostream>
#include "Server.h"

int main() {
    Server *server = NULL;

    try
    {
        server = new Server(3100);
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