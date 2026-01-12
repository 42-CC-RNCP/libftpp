#pragma once
#include "acceptor.hpp"
#include "tcp_transport.hpp"
#include <arpa/inet.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

class TCPAcceptor : public IAcceptor
{
public:
    TCPAcceptor();
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
    }

    std::unique_ptr<IStreamTransport> tryAccept() override
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd =
            ::accept(listen_fd_, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            return nullptr; // No pending connection
        }
        return std::make_unique<TCPTransport>(client_fd);
    }

    int nativeHandle() const override { return listen_fd_; }

private:
    int listen_fd_{-1};
};
