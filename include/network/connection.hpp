#pragma once
#include "client_handle.hpp"
#include "data_structures/data_buffer.hpp"
#include "dispatcher.hpp"
#include "i_message_codec.hpp"
#include "i_stream_transport.hpp"

// handle a connection all I/O operations
class Connection
{
public:
    IStreamTransport& transport;
    IMessageCodec& codec;
    Dispatcher& dispatcher;
    DataBuffer readBuf;
    DataBuffer writeBuf;
    ClientHandle handle;

    Connection(IStreamTransport& t,
               IMessageCodec& c,
               Dispatcher& d,
               ClientHandle h) :
        transport(t), codec(c), dispatcher(d), handle(h)
    {
    }

    // OS/socket is ready to read (e.g. epoll IN event)
    void onReadable()
    {
        std::byte buf[4096];
        // read data from transport to readBuf
        size_t n = transport.recvBytes(buf, sizeof(buf));

        if (n < 0) {
            // handle error
            throw std::runtime_error("Error reading from transport");
        }
        else if (n == 0) {
            // connection closed
            transport.disconnect();
            return;
        }
        // append received data to read data buffer
        readBuf << buf;
    }

    // OS/socket is ready to write (e.g. epoll OUT event)
    void onWritable()
    {
        if (writeBuf.size() == 0) {
            return; // nothing to write
        }

        // send data from writeBuf to transport
        size_t n = transport.sendBytes(writeBuf.data(), writeBuf.size());

        if (n < 0) {
            // handle error
            throw std::runtime_error("Error writing to transport");
        }
        else if (n == 0) {
            // connection closed
            transport.disconnect();
            return;
        }
        // remove sent data from write buffer
        writeBuf.seek(n);

        // compact the write buffer if more than half is consumed
        if (writeBuf.tell() > writeBuf.size() / 2) {
            writeBuf.compact();
        }
    }
};
