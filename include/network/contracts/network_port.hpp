// network/contracts/network_port.hpp
#pragma once
#include "network/core/message.hpp"

struct INetworkPort
{
    using ClientId = long long;

    // Send a message to a specific client
    // use right reference or reference to avoid copy huge object
    virtual void sendTo(const Message& msg, ClientId id) = 0;
    // but `send to all` semantics should copy to avoid modification of original
    virtual void sendToAll(const Message& msg) = 0;
    // Disconnect a specific client
    virtual void disconnect(ClientId id) = 0;
    virtual ~INetworkPort() {}
};
