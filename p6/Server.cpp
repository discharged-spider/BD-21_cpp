#include "Server.h"

#include "NetworkTools.h"
#include "Worker.h"

#include <sys/epoll.h>

#include <string>

#include <system_error>
#include <cstring>
#include <bits/unique_ptr.h>
#include <fcntl.h>
#include <iostream>

#include <algorithm>

#include <map>

#define DEBUG(code) code
//#define DEBUG(code)

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
    start_trash ();

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

        if (N == -1) continue;

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

                    DEBUG (cout << "accepted connection" << endl);

                    process (input_fd);
                }

                continue;
            }
        }
    }

    close (server_fd);
}

int Server::accept_client()
{
    struct sockaddr in_addr;
    socklen_t in_len;

    in_len = sizeof in_addr;
    int input_fd = accept (server_fd, &in_addr, &in_len);

    return input_fd;
}

void Server::add_workers (size_t n)
{
    if (n > 1)
    {
        for (int i = 0; i < n; i++)
        {
            add_workers (1);
        }

        return;
    }

    int fd[2];
    static const int parentsocket = 0;
    static const int childsocket = 1;

    socketpair(PF_LOCAL, SOCK_STREAM, 0, fd);

    auto pid = fork();
    if (pid == 0)
    {
        DEBUG (cout << "Add worker" << pid << endl);

        close(fd[parentsocket]);

        Worker w (fd[childsocket], table);

        try
        {
            w.start ();
        }
        catch (std::runtime_error error)
        {
            cout << "Worker error" << endl;
            cout << error.what();

            perror ("WTF");

            _exit (1);
        }
        catch (...)
        {
            cout << "Unknown worker error" << endl;
            cout << std::system_error (errno, std::system_category()).what();

            _exit (1);
        }

        cout << "Worker done" << endl;

        _exit (0);
    }

    close(fd[childsocket]);

    workers_fd.push_back (fd[parentsocket]);
}

void Server::process (int client_fd)
{
    struct msghdr msg;

    /*allocate memory to 'msg_control' field in msghdr struct */
    char buf[CMSG_SPACE(sizeof(int))];
    /*the memory to be allocated should include data + header..
    this is calculated by the above macro...(it merely adds some
    no. of bytes and returs that number..*/

    struct cmsghdr *cmsg;

    struct iovec ve;
    /*must send/receive atleast one byte...
    main purpose is to have some error
    checking.. but this is completely
    irrelevant in the current context..*/

    char *st = (char*)"I";
    /*jst let us allocate 1 byte for formality
    and leave it that way...*/
    ve.iov_base = st;
    ve.iov_len =1;

    /*attach this memory to our main msghdr struct...*/
    msg.msg_iov = &ve;
    msg.msg_iovlen = 1;

    /*these are optional fields ..
    leave these fields with zeros..
    to prevent unnecessary SIGSEGVs..*/
    msg.msg_name = NULL;
    msg.msg_namelen = 0;


    /*here starts the main part..*/
    /*attach the 'buf' to msg_control..
    and fill in the size field correspondingly..
    */

    msg.msg_control = buf;
    msg.msg_controllen = sizeof(buf);

    /*actually msg_control field must
    point to a struct of type 'cmsghdr'
    we just allocated the memory, yet we need to
    set all the corresponding fields..
    It is done as follows:
    */
    cmsg = CMSG_FIRSTHDR(&msg);
    /* this macro returns the address in the buffer..
    from where the first header starts..
    */

    /*set all the fields appropriately..*/
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(client_fd));
    /*in the above field we need to store
    the size of header + data(in this case 4 bytes(int) for our fd..
    this is returned by the 'CMSG_LEN' macro..*/

    *(int*)CMSG_DATA(cmsg) = client_fd;
    /*after the above three fields we keep the actual data..
    the macro 'CMSG_DATA' returns pointer to this location
    and we set it to the file descriptor to be sent..
    */

    msg.msg_controllen = cmsg->cmsg_len;
    /*now that we have filled the 'cmsg' struct
    we store the size of this struct..*/
    /*this one isn't required when you
    pass a single fd..
    but useful when u pass multiple fds.*/

    msg.msg_flags = 0;
    /*leave the flags field zeroed..*/

    unsigned int worker = (unsigned int) (rand() % workers_fd.size());

    auto res = sendmsg (workers_fd [worker], &msg, 0);
    if (res < 0)
    {
        cout << "Worker error" << endl;
        workers_fd.erase (workers_fd.begin() + worker);
    }

    close (client_fd);
}

void Server::start_trash()
{
    auto pid = fork();
    if (pid == 0)
    {
        table.clear_thread();
        exit (0);
    }
}

#undef MAX_EVENTS

