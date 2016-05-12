#ifndef P3_SERVER_H
#define P3_SERVER_H

#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>

#include <vector>

#include "Data.h"
#include "ClientShell.h"

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

    size_t workers_;

    Data data_;

public:
    typedef Server self_type;

    Server (unsigned short port, const size_t workers_n) :
        io_service_ (),
        acceptor_ (io_service_, tcp::endpoint(tcp::v4(), port)),

        workers_ (workers_n),

        data_ ()
    {
        acceptor_.set_option(boost::asio::socket_base::reuse_address(true));
    }

    void run ();

    void start_accept ();
    void end_accept (ClientShell::pointer connection, const error_code &error);
};

#endif //P3_SERVER_H
