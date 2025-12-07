#pragma once
#include "connection.hpp"
#include "dispatcher.hpp"
#include "i_message_codec.hpp"
#include "i_stream_transport.hpp"
#include "message.hpp"
#include <functional>

// a orchestrator, not responsible for low-level details
class Client
{
public:
    Client(IStreamTransport& t, IMessageCodec& c);

    void connect(const std::string& addr, std::size_t port)
    {
        Endpoint ep{addr, static_cast<std::uint16_t>(port)};

        // use transport to connect
        // it doesn't need to know how to connect
        connection_.transport.connect(ep);
    }

    void send(const Message& msg);
    void update(); // recv -> codec -> dispatcher

    template <typename T>
    void defineAction(Message::Type t, std::function<void(const T&)> h);

private:
    Connection connection_;
    Dispatcher dispatcher_;
};
