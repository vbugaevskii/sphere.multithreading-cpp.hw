#include <iostream>
#include "Client.h"

int main() {
    Client *client = NULL;

    try
    {
        client = new Client(3100);
        client->start();
        delete client;
    }
    catch (const std::system_error& error)
    {
        if (client)
        {
            delete client;
            client = NULL;
        }
        std::cout << "Error: " << error.code()
                  << " - " << error.code().message() << '\n';
    }
    catch (char const *error)
    {
        if (client)
        {
            delete client;
            client = NULL;
        }
        std::cout << error << std::endl;
    }

    return 0;
}