#pragma once
#include "acceptor.hpp"
#include "tcp_transport.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

static void set_nonblocking(int fd)
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

class TCPAcceptor : public IAcceptor
{
public:
    TCPAcceptor() = default;
    ~TCPAcceptor() override
    {
        if (listen_fd_ >= 0) {
            ::close(listen_fd_);
        }
    }

    void listen(std::uint16_t port) override
    {
        listen_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        if (listen_fd_ < 0) {
            throw std::runtime_error("Failed to create socket");
        }

        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port);

        // allow address reuse
        int yes = 1;
        ::setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

        if (::bind(
                listen_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr))
            < 0) {
            ::close(listen_fd_);
            throw std::runtime_error("Failed to bind socket");
        }

        if (::listen(listen_fd_, SOMAXCONN) < 0) {
            ::close(listen_fd_);
            throw std::runtime_error("Failed to listen on socket");
        }

        try {
            set_nonblocking(listen_fd_);
        }
        catch (...) {
            ::close(listen_fd_);
            listen_fd_ = -1;
            throw;
        }
    }

    std::unique_ptr<IStreamTransport> tryAccept() override
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        for (;;) {
            int client_fd =
                ::accept(listen_fd_, (sockaddr*)&client_addr, &client_len);
            if (client_fd >= 0) {
                return std::make_unique<TCPTransport>(client_fd);
            }

            if (errno == EINTR) {
                continue; // interrupted, retry
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return nullptr; // no pending connections
            }

            throw std::runtime_error(std::string("accept failed: ")
                                     + std::strerror(errno));
        }
    }

    int nativeHandle() const override { return listen_fd_; }

private:
    int listen_fd_{-1};
};
