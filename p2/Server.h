#ifndef P2_SERVER_H
#define P2_SERVER_H

#include <netdb.h>
#include <netinet/in.h>

#include <vector>
#include <unistd.h>

#include <string>

using std::string;

struct ClientData
{
    string stream = "";
    //bool send = false;
};

class Server
{
private:
    int port;
    int server_fd;
    struct sockaddr_in serv_addr;

    void init_connection();
    int accept_client();
public:
    Server(int port_ = 3100) :
        port (port_),
        server_fd(-1),
        serv_addr ()
    {}

    ~Server ()
    {
        close (server_fd);
    }

    void start();
};


#endif //P2_SERVER_H
