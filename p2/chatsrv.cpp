#include <iostream>

#include "Server.h"

using std::cout;
using std::endl;

int main()
{
    try
    {
        Server server (3100);

        server.start();
    }
    catch (std::runtime_error error)
    {
        cout << "Error" << endl;
        cout << error.what();

        return 1;
    }
    catch (...)
    {
        cout << "Error" << endl;

        return 1;
    }

    return 0;
}
