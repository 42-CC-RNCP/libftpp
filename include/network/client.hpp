#pragma once
#include "connection.hpp"
#include "dispatcher.hpp"
#include "message.hpp"
#include "message_codec.hpp"
#include "outbound_port.hpp"
#include "stream_transport.hpp"
#include <cstdio>
#include <functional>
#include <stdexcept>

// a orchestrator, not responsible for low-level details
class Client;
using ClientDispatcher = DispatcherT<OutboundPort>;
using ClientHandler = std::function<void(OutboundPort&, const Message&)>;

class Client
{
public:
    Client(IStreamTransport& transport, IMessageCodec& codec) :
        dispatcher_(),
        connection_{transport, codec},
        outbound_port_{connection_}
    {
    }

    void connect(const std::string& addr, std::size_t port)
    {
        Endpoint ep{addr, static_cast<std::uint16_t>(port)};

        // use transport to connect
        // client doesn't need to know how to connect
        connection_.connect(ep);
    }

    void send(const Message& msg)
    {
        // encode the message and write to the write buffer
        connection_.queue(msg);

        // // notify the transport that there is data to write
        // connection_.onWritable();
    }

    // recv -> codec -> dispatcher
    void update()
    {
        // notify the connection that there is data to read
        // read data from transport
        auto r = connection_.onReadable();

        if (r.status == Connection::IoStatus::Closed) {
            return; // connection closed, nothing to do
        }
        else if (r.status == Connection::IoStatus::Error) {
            throw std::runtime_error("I/O error: " + r.error_msg);
        }

        // try to decode messages from read buffer in non-blocking way
        while (true) {
            Message msg(0);
            auto r = connection_.tryDecode(msg);

            if (r.status == DecodeStatus::Ok) {
                // dispatch the decoded message to the appropriate handler
                dispatcher_.dispatch(outbound_port_, msg);
            }
            else if (r.status == DecodeStatus::NeedMoreData) {
                // not enough data to decode a complete message
                break;
            }
            else if (r.status == DecodeStatus::Invalid) {
                // handle decoding error
                throw std::runtime_error("Decoding error: " + r.error_msg);
            }
        }
    }

    void defineAction(Message::Type& t, ClientHandler& h)
    {
        dispatcher_.defineRawAction(t, h);
    }

private:
    ClientDispatcher dispatcher_;
    Connection connection_;
    OutboundPort outbound_port_;
};
