#ifndef P3_PIPE_H
#define P3_PIPE_H

#include <iostream>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <string>
#include "Connection.h"

using std::cout;
using std::endl;

//#define DEBUG(CODE) {CODE;}
#define DEBUG(CODE)

using namespace boost::asio;

using boost::asio::io_service;
using boost::asio::ip::tcp;

using boost::system::error_code;

using std::string;

#define MAP_1(func, arg1) boost::bind(&self_type::func, shared_from_this (), arg1)
#define MAP_2(func, arg1, arg2) boost::bind(&self_type::func, shared_from_this (), arg1, arg2)

#define BUFF_SIZE 1024

class Connection;

class Pipe : public boost::enable_shared_from_this <Pipe>
{
private:
    boost::shared_ptr<Connection> base_;

    tcp::socket& from_;
    tcp::socket& to_;

    string msg;

    bool write_avaliable_ = false;
    bool write_free_      = true;

    char read_buf_  [BUFF_SIZE];
    char write_buf_ [BUFF_SIZE];

    Pipe (boost::shared_ptr<Connection> base, tcp::socket& from, tcp::socket& to) :
        base_ (base),

        from_ (from),
        to_ (to)
    {}

public:
    typedef Pipe self_type;
    typedef boost::shared_ptr<Pipe> pointer;

    static pointer create (boost::shared_ptr<Connection> base, tcp::socket& from, tcp::socket& to)
    {
        return pointer (new Pipe (base, from, to));
    }

    //all ok
    ~Pipe ()
    {
        DEBUG (cout << "pipe_deleted" << endl;)

        from_.close ();
        to_.close ();
    }

    void start ()
    {
        add_read ();
    }

    void start_output()
    {
        write_avaliable_ = true;
        add_write ();
    }

    void read_complete (const error_code & err, size_t bytes)
    {
        DEBUG (cout << "read_complete" << endl;)

        if (err)
        {
            if (msg != "")
            {
                add_write ();
            }

            DEBUG (cout << "read_error_end" << endl;)

            from_.close ();

            return;
        }

        DEBUG (cout << "read_ok" << endl;)

        msg.append (read_buf_, bytes);

        if (msg != "")
        {
            add_write ();
        }

        add_read ();
    }

    void write_complete (const error_code & err, size_t bytes)
    {
        DEBUG (cout << "write_complete" << endl;)

        if (err)
        {
            write_avaliable_ = false;

            DEBUG (cout << "write_error_end" << endl;)

            to_.close ();

            return;
        }

        write_free_ = true;

        if (msg != "") add_write();
    }

    void add_read  ()
    {
        DEBUG (cout << "add_read" << endl;)

        from_.async_read_some (buffer (read_buf_, BUFF_SIZE), MAP_2(read_complete, _1, _2));
    }
    void add_write ()
    {
        DEBUG (cout << "add_write" << endl;)

        if (!write_avaliable_ || !write_free_ || msg == "") return;

        DEBUG (cout << "write_ok" << endl;)

        auto size = msg.size() > BUFF_SIZE? BUFF_SIZE : msg.size();
        for (int i = 0; i < size; i ++)
        {
            write_buf_ [i] = msg [i];
        }

        msg.erase (msg.begin(), msg.begin() + size);

        write_free_ = false;

        to_.async_write_some (buffer (write_buf_, size), MAP_2(write_complete, _1, _2));
    }
};

#endif //P3_PIPE_H
