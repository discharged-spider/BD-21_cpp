#include "Server.h"

void Server::run ()
{
    DEBUG (cout << "run" << endl;)

    start_accept ();
    io_service_.run ();
}

void Server::start_accept ()
{
    DEBUG (cout << "start_accept" << endl;)

    if (workers_.size () > 0)
    {
        unsigned int worker = (unsigned int) (rand() % workers_.size());
        auto connection = Connection::create(io_service_, workers_[worker]);

        //connection -> connect_output();

        acceptor_.async_accept(connection->input_socket(),
                               boost::bind(&self_type::end_accept, this, connection, placeholders::error));
    }
}

void Server::end_accept (Connection::pointer connection, const error_code &error)
{
    DEBUG (cout << "end_accept" << endl;)

    if (!error)
    {
        connection -> on_input_connect ();
        connection -> connect_output ();
    }
    else
    {
        DEBUG (cout << "error on accept" << error << endl;)
    }

    start_accept ();
}
