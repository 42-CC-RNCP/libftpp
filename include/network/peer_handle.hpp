#pragma once
#include "message.hpp"
#include "network_port.hpp"

// the limit functions that server can use to interact with clients
// avoid the server to directly modify the client
class PeerHandle
{
public:
    using PeerId = long long;

    PeerHandle(PeerId id, INetworkPort* port) : id_(id), port_(port) {}

    PeerId id() const;
    void send(Message&& msg);
    void disconnect();

private:
    PeerId id_;
    INetworkPort* port_;
};
