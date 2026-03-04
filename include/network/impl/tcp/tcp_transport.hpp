// network/impl/tcp/tcp_transport.hpp
#pragma once
#include "network/contracts/stream_transport.hpp"
#include "socket_utils.hpp"
#include <arpa/inet.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

class TCPTransport : public IStreamTransport
{
public:
    TCPTransport() : sockfd_(-1), connected_(false) {}
    TCPTransport(int fd) : sockfd_(fd), connected_(fd >= 0) {}
    ~TCPTransport() { disconnect(); }

    void connect(const Endpoint& ep) override
    {
        // create socket
        sockfd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd_ < 0) {
            throw std::runtime_error("Failed to create socket");
        }

        // set up sockaddr_in structure
        struct sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(ep.port);

        if (inet_pton(AF_INET, ep.ip.c_str(), &server_addr.sin_addr) <= 0) {
            ::close(sockfd_);
            throw std::runtime_error("Invalid address: " + ep.ip);
        }

        // connect to server
        if (::connect(
                sockfd_, (struct sockaddr*)&server_addr, sizeof(server_addr))
            < 0) {
            close(sockfd_);
            throw std::runtime_error("Failed to connect to server");
        }
        connected_ = true;
        set_nonblocking(sockfd_);
    }

    void disconnect() override
    {
        if (connected_ && sockfd_ >= 0) {
            ::close(sockfd_);
        }
        sockfd_ = -1;
        connected_ = false;
    }

    ssize_t recvBytes(std::byte* buf, size_t len) override
    {
        if (!connected_) {
            throw std::runtime_error("Not connected");
        }
        return ::recv(sockfd_, buf, len, 0);
    }

    ssize_t sendBytes(const std::byte* data, size_t len) override
    {
        if (!connected_) {
            throw std::runtime_error("Not connected");
        }
        return ::send(sockfd_, data, len, 0);
    }

    bool isConnected() const override { return connected_; }

    int nativeHandle() const override { return sockfd_; }

private:
    int sockfd_;
    bool connected_;
};
