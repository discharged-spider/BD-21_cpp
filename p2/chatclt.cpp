//For Linux Ubuntu

#include <iostream>

#include "Client.h"

using std::cout;
using std::endl;

int main()
{
    try
    {
        Client client(3100);

        client.start();
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
