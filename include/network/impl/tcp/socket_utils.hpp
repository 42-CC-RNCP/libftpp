// network/impl/tcp/socket_utils.hpp
#pragma once
#include <cstring>
#include <fcntl.h>
#include <stdexcept>

inline void set_nonblocking(int fd)
{
    int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        throw std::runtime_error(std::string("fcntl(F_GETFL) failed: ")
                                 + std::strerror(errno));
    }
    if (::fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        throw std::runtime_error(std::string("fcntl(F_SETFL) failed: ")
                                 + std::strerror(errno));
    }
}
