#pragma once
#include "message.hpp"
#include <functional>
#include <utility>

template <typename Context>
using RawHandlerT = std::function<void(Context&, const Message&)>;

template <typename Context> class DispatcherT
{
public:
    void defineRawAction(Message::Type t, RawHandlerT<Context> h)
    {
        if (handlers_.count(t)) {
            throw std::runtime_error("Handler already defined");
        }
        handlers_[t] = std::move(h);
    }

    void dispatch(Context& ctx, const Message& msg) const
    {
        auto it = handlers_.find(msg.type());
        if (it == handlers_.end()) {
            throw std::runtime_error("No handler defined");
        }
        it->second(ctx, msg);
    }

private:
    std::unordered_map<Message::Type, RawHandlerT<Context>> handlers_;
};
