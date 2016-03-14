#ifndef P3_SERVER_H
#define P3_SERVER_H

#include <boost/asio.hpp>
#include <vector>

#include "Connection.h"

using namespace boost::asio;

using boost::asio::io_service;
using boost::asio::ip::tcp;

using boost::system::error_code;

using std::vector;

class Server
{
private:
    io_service io_service_;
    tcp::acceptor acceptor_;

    vector <tcp::endpoint> workers_;

public:
    typedef Server self_type;

    Server (unsigned short port, const vector <tcp::endpoint>& workers) :
        io_service_ (),
        acceptor_ (io_service_, tcp::endpoint(tcp::v4(), port)),

        workers_ (workers)
    {
        acceptor_.set_option(boost::asio::socket_base::reuse_address(true));
    }

    void run ();

    void start_accept ();
    void end_accept (Connection::pointer connection, const error_code& error);
};

#endif //P3_SERVER_H
