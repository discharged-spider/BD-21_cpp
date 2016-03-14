#include <iostream>
#include <fstream>

#include "Server.h"

using namespace std;

class my_ctype : public
                 std::ctype<char>
{
    mask my_table[table_size];
public:
    my_ctype(size_t refs = 0)
        : std::ctype<char>(&my_table[0], false, refs)
    {
        std::copy_n(classic_table(), table_size, my_table);
        my_table[','] = (mask)space;
    }
};

std::istream &operator>>(std::istream &is, tcp::endpoint &addres);

int main (int argn, const char** args)
{
    if (argn <= 1)
    {
        cout << "Bad input format" << endl;

        return 1;
    }

    ifstream config;
    config.open (args [1]);

    if (!config.is_open())
    {
        cout << "No config file" << endl;

        return 1;
    }

    std::locale x(std::locale::classic(), new my_ctype);
    config.imbue(x);

    unsigned short port = 0;
    vector <tcp::endpoint> workers;

    try
    {
        config >> port;
        tcp::endpoint worker;
        while (config >> worker)
        {
            workers.push_back (worker);
        }
    }
    catch (std::exception& error)
    {
        cout << "Bad config" << endl;
        cout << error.what () << endl;

        return 1;
    }

    config.close ();

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

std::istream &operator>>(std::istream &is, tcp::endpoint &addr)
{
    istream::sentry s (is, true);
    if(!s) return is;

    //127.0.0.1
    string addr_s = "";

    while (is.good ())
    {
        char c = 0;
        is >> c;

        if (c == ':') break;
        addr_s += c;
    }

    if (addr_s == "")
    {
        is.setstate(ios_base::failbit);
        return is;
    }

    int port = 0;
    if (!(is >> port))
    {
        is.setstate(ios_base::failbit);
        return is;
    }

    addr = tcp::endpoint (ip::address::from_string (addr_s), (unsigned short)port);

    return is;
}