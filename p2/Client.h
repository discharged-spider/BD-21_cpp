#ifndef P2_CLIENT_H
#define P2_CLIENT_H

#include <netdb.h>
#include <netinet/in.h>

#include <unistd.h>

class Client
{
private:
    int server_port;
    int client_port;

    int client_fd;
    struct sockaddr_in client_addr;

    void init_connection();
public:
    Client(int server_port_ = 3100, int client_port_ = 0) :
            server_port(server_port_),
            client_port(client_port_),
            client_fd(-1),
            client_addr()
    {}

    ~Client()
    {
        close(client_fd);
    }

    void start();
};

#endif //P2_CLIENT_H
