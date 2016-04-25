#include <iostream>
#include "Server.h"

using namespace std;

int main()
{
    try
    {
        Server server(3100);

        server.add_workers (1);

        server.start();
    }
    catch (std::runtime_error error)
    {
        cout << "Server error" << endl;
        cout << error.what();

        return 1;
    }
    catch (...)
    {
        cout << "Server error" << endl;
        cout << std::system_error (errno, std::system_category()).what();

        return 1;
    }

    return 0;

    /*
    SharedHashTable table;

    item_t item;

    for (int i = 0; i < 20; i ++)
    {
        item.set (std::to_string (i % 10) + std::to_string (i), 10, string ("pony") + std::to_string(i));
        cout << table.add (item) << std::endl;
    }

    int i = 0;
    i = 1;
    item.set (std::to_string (i % 10) + std::to_string (i), 10, string ("lol"));
    cout << table.get (item) << endl;

    std::cout << string (item.value) << endl;

    table.print();

    i = 2;
    item.set (std::to_string (i % 10) + std::to_string (i), 10, string ("lol"));
    table.del(item);

    table.print();

    i = 12;
    item.set (std::to_string (i % 10) + std::to_string (i), 10, string ("lol"));
    cout << table.get (item) << endl;

    std::cout << string (item.value) << endl;

    return 0;
    */
}