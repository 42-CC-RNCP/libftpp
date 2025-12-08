#pragma once
#include "client_handle.hpp"
// #include "data_structures/data_buffer.hpp"
#include "message.hpp"
#include <functional>
#include <utility>

using RawHandler = std::function<void(ClientHandle&, const Message&)>;

// map message types to their handlers
class Dispatcher
{
public:
    void defineRawAction(Message::Type t, RawHandler h)
    {
        if (handlers_.count(t)) {
            throw std::runtime_error(
                "Handler for this message type already defined");
        }
        handlers_[t] = std::move(h);
    }

    // // define a handler for a specific message type T
    // template <typename T>
    // void
    // defineTypedAction(Message::Type type,
    //                   std::function<void(ClientHandle&, const T&)>
    //                   typedHandler)
    // {
    //     defineRawAction(type,
    //                     [typedHandler](ClientHandle& cli, const Message& msg)
    //                     {
    //                         T obj;

    //                     });
    // }

    void dispatch(ClientHandle& cli, const Message& msg) const
    {
        auto it = handlers_.find(msg.type());
        if (it != handlers_.end()) {
            it->second(cli, msg);
        }
        else {
            throw std::runtime_error(
                "No handler defined for this message type");
        }
    }

private:
    std::unordered_map<Message::Type, RawHandler> handlers_;
};
