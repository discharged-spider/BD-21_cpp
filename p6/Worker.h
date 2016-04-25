#ifndef P2_CLIENT_H
#define P2_CLIENT_H

#include <netdb.h>
#include <netinet/in.h>

#include <unistd.h>

#include <string>
#include "SharedHashTable.h"

using std::string;

struct ClientData
{
    string stream = "";
    //bool send = false;
};

class Worker
{
private:
    int root_fd;

    SharedHashTable& table;

public:
    Worker(int root_fd, SharedHashTable& table_) :
        root_fd (root_fd),
        table (table_)
    {}

    ~Worker()
    {
        close (root_fd);
    }

    void start();

    string process_command (string msg);
    string process_get (const string &key_);
    string process_set (const string &key_, const ttl_t ttl_, const string &value_);
};

#endif //P2_CLIENT_H