#include "Server.h"

#include "NetworkTools.h"

#include <sys/epoll.h>

#include <string>

#include <system_error>
#include <cstring>
#include <bits/unique_ptr.h>
#include <fcntl.h>
#include <iostream>

#include <algorithm>

#include <map>

using std::cout;
using std::endl;

using std::map;

void Server::init_connection()
{
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0)
    {
        throw std::system_error(errno, std::system_category());
    }

    /* Initialize socket structure */
    bzero((char *) &serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int yes = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0)
    {
        throw std::system_error(errno, std::system_category());
    }

    if (bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        throw std::system_error(errno, std::system_category());
    }

    if (make_socket_non_blocking(server_fd) < 0)
    {
        throw std::system_error(errno, std::system_category());
    }

    if (listen(server_fd, SOMAXCONN)  < 0)
    {
        throw std::system_error(errno, std::system_category());
    }
}

#define MAX_EVENTS 64

void Server::start()
{
    map <int, ClientData> data;

    init_connection();

    int Epoll = epoll_create1(0);

    struct epoll_event event;
    event.data.fd = server_fd;
    event.events = EPOLLIN; //| EPOLLET

    if (epoll_ctl(Epoll, EPOLL_CTL_ADD, server_fd, &event) < 0)
    {
        throw std::system_error(errno, std::system_category());
    }

    std::unique_ptr <epoll_event> events (new epoll_event [MAX_EVENTS]);

    while(true)
    {
        int N = epoll_wait(Epoll, (epoll_event *)events.get(), MAX_EVENTS, -1);
        for (unsigned int i = 0; i < N; i ++)
        {
            epoll_event cur_event = ((epoll_event *)events.get()) [i];

            if (server_fd == cur_event.data.fd)
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
                    int input_fd = accept_client();
                    if (input_fd == -1)
                    {
                        if ((errno == EAGAIN) ||
                            (errno == EWOULDBLOCK))
                        {
                            /* We have processed all incoming
                               connections. */
                            break;
                        }

                        throw std::system_error(errno, std::system_category());
                    }

                    cout << "accepted connection" << endl;
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

                    string message = "Welcome socket_#";
                    message += std::to_string(input_fd);
                    message += "!\n";

                    auto buf_msg = message.c_str();
                    auto len = message.size();

                    if (send (input_fd, buf_msg, len, 0) != len)
                    {
                        cout << "connection terminated  (bad welcome)" << endl;

                        data.erase (cur_event.data.fd);
                        close (cur_event.data.fd);
                    }
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

                    data [input_fd].stream.append(buf, count);
                }

                string& str = data [input_fd].stream;
                str.erase (std::remove(str.begin(), str.end(), '\r'), str.end());

                string start = "from ";
                start += std::to_string(input_fd);
                start += ":";

                while (true)
                {
                    auto pos = str.find('\n');
                    if (pos == std::string::npos) break;

                    string message = str.substr(0, pos);
                    str.erase(str.begin(), str.begin() + pos + 1);

                    //cout << '[' << message << ']' << str << '|' << endl;

                    if (message == "") continue;

                    cout << message << endl;

                    message += '\n';

                    message = start + message;

                    auto buf_msg = message.c_str();
                    auto len = message.size();
                    for (auto pair : data)
                    {
                        int client_fd = pair.first;
                        if (send (client_fd, buf_msg, len, 0) != len)
                        {
                            cout << "connection terminated (while send)" << endl;

                            data.erase (cur_event.data.fd);
                            close (cur_event.data.fd);
                        }
                    }
                }

                if (closed)
                {
                    cout << "connection terminated  (after read)" << endl;

                    data.erase (input_fd);
                    close (input_fd);

                    continue;
                }
            }

            if (cur_event.events & EPOLLERR)
            {
                cout << "connection terminated (some error)" << endl;

                data.erase (cur_event.data.fd);
                close (cur_event.data.fd);

                continue;
            }

            if (cur_event.events & EPOLLHUP)
            {
                cout << "connection terminated (usual hang up)" << endl;

                data.erase (cur_event.data.fd);
                close (cur_event.data.fd);

                continue;
            }
        }
    }
}

int Server::accept_client()
{
    struct sockaddr in_addr;
    socklen_t in_len;

    in_len = sizeof in_addr;
    int input_fd = accept (server_fd, &in_addr, &in_len);

    return input_fd;
}

#undef MAX_EVENTS

