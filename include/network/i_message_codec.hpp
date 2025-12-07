#pragma once
#include "data_structures/data_buffer.hpp"
#include "message.hpp"
#include <string>

enum class DecodeStatus
{
    Ok,           // received a complete message
    NeedMoreData, // need to wait for more data to decode
    Invalid       // invalid format
};

struct DecodeResult
{
    DecodeStatus status;
    int error_code = 0;
    std::string error_msg;
};

// to decode the boundary of a message
class IMessageCodec
{
public:
    virtual void encode(const Message& msg, DataBuffer& out) = 0;
    virtual DecodeResult tryDecode(DataBuffer& in, Message& outMsg) = 0;
    virtual ~IMessageCodec() {}
};
