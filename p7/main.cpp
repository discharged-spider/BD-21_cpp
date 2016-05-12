#include <iostream>
#include "Server.h"

using std::cout;
using std::endl;

const unsigned short port = 4000;
const unsigned short workers = 10;

int main()
{
    try
    {
        Server server (port, workers);

        server.run ();
    }
    catch (std::exception& error)
    {
        cout << "Server error" << endl;
        cout << error.what () << endl;

        return 1;
    }

    return 0;
}