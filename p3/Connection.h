#ifndef P3_CONNECTION_H
#define P3_CONNECTION_H

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/thread/thread.hpp

#include <string>

#include "Pipe.h"

using namespace boost::asio;

using boost::asio::io_service;
using boost::asio::ip::tcp;

using boost::system::error_code;

using std::string;

class Connection : public boost::enable_shared_from_this<Connection>
{
private:
    tcp::endpoint worker_;
    tcp::socket input_;
    tcp::socket output_;

    boost::weak_ptr <Pipe> right_;
    boost::weak_ptr <Pipe> left_;

    Connection (io_service& io_service_, tcp::endpoint worker) :
        worker_ (worker),
        input_ (io_service_),
        output_ (io_service_),
        left_ (),
        right_ ()
    {}
public:
    typedef Connection self_type;
    typedef boost::shared_ptr<Connection> pointer;

    static pointer create (io_service& io_service_, tcp::endpoint worker)
    {
        return pointer (new Connection (io_service_, worker));
    }

    //All ok
    ~Connection()
    {
        DEBUG (cout << "destructed" << endl;)

        input_.close();
        output_.close();
    }

    void connect_output()
    {
        output_.async_connect (worker_, MAP_1(on_output_connect, _1));
    }

    void on_input_connect ()
    {
        DEBUG (cout << "on_input_connect" << endl;)

        DEBUG (cout << "create_right" << endl;)

        auto right = Pipe::create (shared_from_this(), input_, output_);
        right -> start ();

        if (!left_._empty())
        {
            right -> start_output ();
        }

        right_ = right -> weak_from_this();
    }

    void on_output_connect (const error_code & err)
    {
        DEBUG (cout << "on_output_connect" << endl;)

        if (err)
        {
            //DO THMTH?
            this -> ~Connection ();
            return;
        }

        DEBUG (cout << "init_left" << endl;)

        auto left = Pipe::create (shared_from_this(), output_, input_);
        left -> start ();
        left -> start_output();

        left_ = left -> weak_from_this();

        DEBUG (cout << "start right" << endl;)

        auto right = right_.lock ();
        if (right)
        {
            right -> start_output ();
        }
    }

    tcp::socket& input_socket ()
    {
        return input_;
    }
};

#endif //P3_CONNECTION_H
