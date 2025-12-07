#pragma once
#include "connection.hpp"
#include "dispatcher.hpp"
#include "i_network_port.hpp"
#include "message.hpp"
#include <functional>
#include <unordered_map>
#include <vector>

// a orchestrator for multiple clients, not responsible for low-level details
// inherits INetworkPort to provide sendTo and sendToAll functionalities
class Server : public INetworkPort
{
public:
    void start(const size_t& p_port);
    void defineAction(const Message::Type& messageType,
                      const std::function<void(ClientId& clientID,
                                               const Message& msg)>& action);
    void sendTo(const Message& message, ClientId clientID) override;
    void sendToAll(const Message& message) override;
    void sendToArray(const Message& message, std::vector<ClientId> clientIDs);
    void update();

private:
    std::unordered_map<ClientId, Connection> connections_;
    Dispatcher dispatcher_;
};
