// network/components/peer_handle.hpp
#pragma once
#include "network/contracts/network_port.hpp"
#include "network/core/message.hpp"

// the limit functions that server can use to interact with clients
// avoid the server to directly modify the client
class PeerHandle
{
public:
    using PeerId = long long;

    PeerHandle(PeerId id, INetworkPort* port) : id_(id), port_(port) {}

    PeerId id() const { return id_; }
    void send(Message&& msg) { port_->sendTo(msg, id_); }
    void disconnect() { port_->disconnect(id_); }

private:
    PeerId id_;
    INetworkPort* port_;
};
