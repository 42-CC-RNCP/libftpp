#pragma once
#include "i_network_port.hpp"
#include "message.hpp"

// the limit functions that server can use to interact with clients
class ClientHandle
{
public:
    using ClientId = long long;
    ClientHandle(ClientId id, INetworkPort* port) : id_(id), port_(port) {}

    ClientId id() const { return id_; }
    void send(Message&& msg) { port_->sendTo(id_, std::move(msg)); }
    void disconnect() { port_->disconnect(id_); }

private:
    ClientId id_;
    INetworkPort* port_;
};
