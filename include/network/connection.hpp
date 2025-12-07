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
    DataBuffer readBuf;
    DataBuffer writeBuf;
    IMessageCodec& codec;
    Dispatcher& dispatcher;
    ClientHandle handle;

    void onReadable();
    void onWritable();
};
