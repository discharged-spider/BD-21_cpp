#ifndef P2_NETWORKTOOLS_H
#define P2_NETWORKTOOLS_H

#include <bits/unique_ptr.h>
#include <fcntl.h>
#include <system_error>

#define BUF_SIZE 1024

static int make_socket_non_blocking (int sfd)
{
    int flags = fcntl (sfd, F_GETFL, 0);
    if (flags == -1)
    {
        throw std::system_error(errno, std::system_category());
    }

    flags |= O_NONBLOCK;
    int s = fcntl (sfd, F_SETFL, flags);
    if (s == -1)
    {
        throw std::system_error(errno, std::system_category());
    }

    return 0;
}

#endif //P2_NETWORKTOOLS_H
