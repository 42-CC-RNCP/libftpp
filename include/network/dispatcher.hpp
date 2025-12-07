#pragma once
#include "client_handle.hpp"
#include "message.hpp"
#include <functional>

using RawHandler = std::function<void(ClientHandle&, const Message&)>;

// map message types to their handlers
class Dispatcher
{
public:
    void defineRawAction(Message::Type t, RawHandler h);

    // define a handler for a specific message type T
    template <typename T>
    void defineTypedAction(Message::Type t,
                           std::function<void(ClientHandle&, const T&)> h);
    void dispatch(ClientHandle& cli, const Message& msg);

private:
    std::unordered_map<Message::Type, RawHandler> handlers_;
};
