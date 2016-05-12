#include "Server.h"

void Server::run ()
{
    DEBUG (cout << "run" << endl;)

    start_accept ();

    boost::thread_group threads;

    for (int i = 0; i < workers_; i ++)
    {
        threads.create_thread (boost::bind(&io_service::run, &io_service_));
    }

    threads.join_all ();
}

void Server::start_accept ()
{
    DEBUG (cout << "start_accept" << endl;)

    auto connection = ClientShell::create (&data_, io_service_);

    acceptor_.async_accept (connection->socket (),
                           boost::bind (&self_type::end_accept, this, connection, placeholders::error));
}

void Server::end_accept (ClientShell::pointer connection, const error_code &error)
{
    DEBUG (cout << "end_accept" << endl;)

    connection -> on_connect ();

    start_accept ();
}


