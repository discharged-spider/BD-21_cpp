#include "Client.h"

#include "NetworkTools.h"

#include <string>
#include <cstring>
#include <iostream>

#include <algorithm>

using std::string;

using std::cin;
using std::cout;
using std::endl;

void Client::init_connection()
{
    client_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (client_fd < 0)
    {
        throw std::system_error(errno, std::system_category());
    }

    /* Initialize socket structure */
    bzero((char *) &client_addr, sizeof(client_addr));

    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(server_port);
    client_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    int yes = 1;
    if (setsockopt(client_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0)
    {
        throw std::system_error(errno, std::system_category());
    }

    if (connect(client_fd, (struct sockaddr *) &client_addr, sizeof(client_addr)) < 0)
    {
        throw std::system_error(errno, std::system_category());
    }

    if (make_socket_non_blocking(client_fd) < 0)
    {
        throw std::system_error(errno, std::system_category());
    }
}

void Client::start()
{
    init_connection();

    fd_set read_flags, write_flags;

    string message = "";
    string input   = "";

    string erase    = "\x1b[2K";
    string up       = "\x1b[1A";
    string down     = "\x1b[1B";
    bool escape = false;

    cout << "#connected" << endl;

    cout << "#unput text any time you want" << endl;

    bool log = false;

    while(true)
    {
        FD_ZERO(&read_flags);
        FD_ZERO(&write_flags);

        FD_SET(client_fd, &read_flags);
        FD_SET(STDIN_FILENO, &read_flags);

        if (message != "") FD_SET(client_fd, &write_flags);

        int sel = select(client_fd+1, &read_flags, &write_flags, (fd_set*)0, nullptr);
        if(sel < 0)
        {
            throw std::system_error(errno, std::system_category());
        }

        if(FD_ISSET(STDIN_FILENO, &read_flags))
        {
            cin >> message;

            if (escape)
            {
                cout << up << erase;
            }

            if (log) cout << "@ input msg [" << message << "]" << endl;
        }

        if(FD_ISSET(client_fd, &read_flags))
        {
            FD_CLR(client_fd, &read_flags);

            char buf[BUF_SIZE];

            bool closed = false;

            while (true)
            {
                auto count = read (client_fd, buf, sizeof buf);
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

                input.append(buf, count);
            }

            string& str = input;
            str.erase (std::remove(str.begin(), str.end(), '\r'), str.end());

            while(true)
            {
                auto pos = str.find('\n');
                if (pos == std::string::npos) break;

                string income_message = str.substr(0, pos);
                str.erase(str.begin(), str.begin() + pos + 1);

                //cout << '[' << message << ']' << str << '|' << endl;

                if (income_message == "") continue;

                if (log) cout << "@ get message [" << income_message << "]" << endl;

                if (escape)
                {
                    cout << up << up << endl << income_message << endl << endl;
                }
                else
                {
                    cout << endl << income_message << endl;
                }
            }

            if (closed)
            {
                break;
            }
        }

        if(FD_ISSET(client_fd, &write_flags))
        {
            FD_CLR(client_fd, &write_flags);

            if (log) cout << " @ send msg [" << message << "]" << endl;

            message += '\n';

            auto buf_msg = message.c_str();
            auto len = message.size();

            if (send (client_fd, buf_msg, len, 0) != len)
            {
                break;
            }

            message = "";
        }
    }

    cout << "#connection closed" << endl;

    close(client_fd);
}
