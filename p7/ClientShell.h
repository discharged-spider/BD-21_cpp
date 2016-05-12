#ifndef P7_CONNECTION_H
#define P7_CONNECTION_H

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <iostream>
#include <fstream>

#include "Data.h"

using std::cout;
using std::endl;

#define DEBUG(CODE) {CODE;}
//#define DEBUG(CODE)

#define MAP_1(func, arg1) boost::bind(&self_type::func, shared_from_this (), arg1)
#define MAP_2(func, arg1, arg2) boost::bind(&self_type::func, shared_from_this (), arg1, arg2)

#define BUFF_SIZE 1024

using namespace boost::asio;

using boost::asio::io_service;
using boost::asio::ip::tcp;
using boost::system::error_code;

using std::string;
using std::ifstream;

class ClientShell : public boost::enable_shared_from_this<ClientShell>
{
private:
    tcp::socket client_;

    string input_;
    string output_;

    Data *data_;

    bool write_free_;

    char read_buf_  [BUFF_SIZE];
    char write_buf_ [BUFF_SIZE];

    ClientShell (Data *data, io_service& io_service_) :
        data_ (data),
        client_ (io_service_),
        write_free_ (true)
    {}
public:
    typedef ClientShell self_type;
    typedef boost::shared_ptr<ClientShell> pointer;

    static pointer create (Data *data, io_service& io_service_)
    {
        return pointer (new ClientShell (data, io_service_));
    }

    //All ok
    ~ClientShell()
    {
        DEBUG (cout << "destructed" << endl;)

        client_.close();
    }

    tcp::socket& socket ()
    {
        return client_;
    }

    void on_connect ();

    string process_command (string command);

    void add_read  ();
    void read_complete (const error_code & err, size_t bytes);

    void add_write ();
    void write_complete (const error_code & err, size_t bytes);
};

#endif //P7_CONNECTION_H
