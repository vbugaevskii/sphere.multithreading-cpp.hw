#include "proxy.h"

int main(int argc, char* argv[])
{
    try
    {
        if (argc != 2)
        {
            std::cerr << "There is no config file. Example: proxy config.txt" << std::endl;
            return 1;
        }

        Proxy proxy(argv[1]);
        proxy.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}