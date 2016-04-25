#ifndef P2_SERVER_H
#define P2_SERVER_H

#include <netdb.h>
#include <netinet/in.h>

#include <vector>
#include <unistd.h>

#include <string>
#include "SharedHashTable.h"

using std::vector;

class Server
{
private:
    uint16_t port;
    int server_fd;
    struct sockaddr_in serv_addr;

    vector<int> workers_fd;

    void init_connection();
    int accept_client();

    SharedHashTable table;
public:
    Server(uint16_t port_ = 3100) :
        port(port_),
        server_fd(-1),
        serv_addr(),
        table ()
    {}

    ~Server()
    {
        close (server_fd);
    }

    void start ();

    void add_workers (size_t n = 1);

    void process (int client_fd);

    void start_trash ();
};


#endif //P2_SERVER_H
