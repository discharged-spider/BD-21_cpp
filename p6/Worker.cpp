#include "Worker.h"

#include "NetworkTools.h"

#include <algorithm>
#include <assert.h>
#include <sys/epoll.h>
#include <map>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#define DEBUG(code) code
//#define DEBUG(code)

using std::cin;
using std::cout;
using std::endl;

using std::vector;
using std::map;

using boost::trim;
using boost::ends_with;
using boost::split;
using boost::is_any_of;

#define MAX_EVENTS 64

void Worker::start()
{
    if (make_socket_non_blocking(root_fd) < 0)
    {
        throw std::system_error(errno, std::system_category());
    }

    map <int, ClientData> data;

    int Epoll = epoll_create1(0);

    struct epoll_event event;
    event.data.fd = root_fd;
    event.events = EPOLLIN; //| EPOLLET

    if (epoll_ctl(Epoll, EPOLL_CTL_ADD, root_fd, &event) < 0)
    {
        throw std::system_error(errno, std::system_category());
    }

    std::unique_ptr <epoll_event> events (new epoll_event [MAX_EVENTS]);

    DEBUG (cout << "started" << endl);

    while(true)
    {
        int N = epoll_wait(Epoll, (epoll_event *)events.get(), MAX_EVENTS, -1);

        if (N == -1) continue;

        for (unsigned int i = 0; i < N; i ++)
        {
            epoll_event cur_event = ((epoll_event *)events.get()) [i];

            if (root_fd == cur_event.data.fd)
            {
                if (  cur_event.events & EPOLLERR ||
                      !(cur_event.events & EPOLLIN))
                {
                    throw std::system_error(errno, std::system_category());
                }

                /* We have a notification on the listening socket, which
                   means one or more incoming connections. */
                while (true)
                {
                    //get real address
                    //{
                    struct msghdr msg;
                    /*do all the unwanted things first...
                    same as the send_fd function..*/
                    struct iovec io;
                    char ptr[1];
                    io.iov_base = ptr;
                    io.iov_len = 1;
                    msg.msg_name = 0;
                    msg.msg_namelen = 0;
                    msg.msg_iov = &io;
                    msg.msg_iovlen = 1;
                    /*-----------------------*/


                    char buf[CMSG_SPACE(sizeof(int))];
                    msg.msg_control = buf;
                    msg.msg_controllen = sizeof(buf);
                    /*reasoning is same..as above*/

                    /*now here comes the main part..*/

                    if(recvmsg (root_fd, &msg, 0) == -1)
                    {
                        //all messages read
                        break;
                    }

                    struct cmsghdr *cm;

                    cm =  CMSG_FIRSTHDR(&msg);
                    /*get the first message header..*/

                    if(cm->cmsg_type != SCM_RIGHTS)
                    {
                        throw std::runtime_error ("Unknown type");
                    }

                    /*if control has reached here.. this means
                    we have got the correct message..and when you
                    extract the fd out of this message
                    this need not be same as the one which was sent..
                    allocating a new fd is all done by the kernel
                    and our job is jst to use it..*/

                    int input_fd = *(int*)CMSG_DATA(cm);
                    //}

                    assert (input_fd >= 0);

                    data [input_fd] = ClientData ();

                    /* Make the incoming socket non-blocking and add it to the
                       list of fds to monitor. */
                    if (make_socket_non_blocking(input_fd) < 0)
                    {
                        throw std::system_error(errno, std::system_category());
                    }

                    event.data.fd = input_fd;
                    event.events = EPOLLIN;
                    if (epoll_ctl (Epoll, EPOLL_CTL_ADD, input_fd, &event) < 0)
                    {
                        throw std::system_error(errno, std::system_category());
                    }

                    DEBUG (cout << "worker accepted connection " << input_fd  << endl);
                }

                continue;
            }

            if (cur_event.events & EPOLLIN)
            {
                /* We have data on the fd waiting to be read. Read and
                   display it. We must read whatever data is available
                   completely, as we are running in edge-triggered mode
                   and won't get a notification again for the same
                   data. */

                int input_fd = cur_event.data.fd;

                bool closed = false;

                char buf[BUF_SIZE];

                while (true)
                {
                    auto count = read (input_fd, buf, sizeof buf);
                    if (count == -1)
                    {
                        /* If errno == EAGAIN, that means we have read all
                           data. So go back to the main loop. */
                        if (errno != EAGAIN)
                        {
                            //perror ("read");
                            //done = 1;
                        }
                        break;
                    }
                    if (count == 0)
                    {
                        /* End of file. The remote has closed the
                           connection. */
                        closed = true;
                        break;
                    }

                    data [input_fd].stream.append(buf, (size_t)count);
                }

                string& str = data [input_fd].stream;
                str.erase (std::remove(str.begin(), str.end(), '\r'), str.end());

                while (true)
                {
                    auto pos = str.find('\n');
                    if (pos == std::string::npos) break;

                    string message = str.substr(0, pos);
                    str.erase(str.begin(), str.begin() + pos + 1);

                    if (message == "") continue;

                    string answer = process_command (message);
                    answer += '\n';

                    auto buf_msg = answer.c_str();
                    auto len = answer.size();
                    if (send (input_fd, buf_msg, len, 0) != len)
                    {
                        DEBUG (cout << "connection terminated " << input_fd << " (while send)" << endl);

                        data.erase (input_fd);
                        close (input_fd);

                        closed = false; //prevent double close

                        break;
                    }
                }

                if (closed)
                {
                    DEBUG (cout << "connection terminated " << input_fd << " (after read)" << endl);

                    data.erase (input_fd);
                    close (input_fd);

                    continue;
                }
            }

            if (cur_event.events & EPOLLERR)
            {
                DEBUG (cout << "connection terminated " << cur_event.data.fd << " (some error)" << endl);

                data.erase (cur_event.data.fd);
                close (cur_event.data.fd);

                continue;
            }

            if (cur_event.events & EPOLLHUP)
            {
                DEBUG (cout << "connection terminated " << cur_event.data.fd << " (usual hang up)" << endl);

                data.erase (cur_event.data.fd);
                close (cur_event.data.fd);

                continue;
            }
        }
    }
}

string Worker::process_command (string command)
{
    DEBUG(cout << command << endl);

    trim (command);

    vector <string> parts;
    split (parts, command, is_any_of (L" "), boost::token_compress_on);

    const size_t command_p = 0;

    if (parts.size() < 1)
    {
        return string ("error \"bad command format\"");
    }

    if (parts [0] == "get")
    {
        const size_t key_p = 1;

        if (parts.size() != 2)
        {
            return string ("error \"bad get format\"");
        }

        return process_get (parts [key_p]);
    }

    if (parts [0] == "set")
    {
        const size_t ttl_p   = 1;
        const size_t key_p   = 2;
        const size_t value_p = 3;

        if (parts.size () != 4)
        {
            return string ("error \"bad set format\"");
        }

        ttl_t ttl = 0;
        try
        {
            ttl = boost::lexical_cast<ttl_t> (parts [ttl_p]);
        }
        catch(boost::bad_lexical_cast& e)
        {
            return string ("error \"bad set format (ttl format)\"");
        }

        return process_set (parts [key_p], ttl, parts [value_p]);
    }

    return string ("error \"bad command format\"");
}

string Worker::process_get(const string &key_)
{
    item_t item;
    item.set (key_);

    auto res = table.get (item);

    if (!res) return string ("error 404 not found");

    return string ("ok ") + item.get ();
}

string Worker::process_set(const string &key_, const ttl_t ttl_, const string &value_)
{
    item_t item;
    item.set (key_, ttl_, value_);

    auto res = table.add (item);

    if (!res) return string ("error table is full");

    return string ("ok ") + item.get ();
}





