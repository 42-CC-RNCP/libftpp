#pragma once
#include "byte_queue_adapter.hpp"
#include "message_codec.hpp"
#include "stream_transport.hpp"

// handle a connection all I/O operations
class Connection
{
public:
    enum class IoStatus
    {
        Ok,
        Closed,
        WouldBlock,
        Error
    };

    struct IoResult
    {
        IoStatus status{IoStatus::Ok};
        std::size_t bytes{0}; // success bytes count
        int sys_errno{0};     // errno if error occurs
        std::string error_msg;
    };

    static constexpr size_t CHUNK_SIZE = 4096;

public:
    Connection(IStreamTransport& t, IMessageCodec& c) : codec_(c), transport_(t)
    {
    }

    void connect(Endpoint& ep) { transport_.connect(ep); }

    void queue(const Message& msg) { codec_.encode(msg, tx_); }

    DecodeResult tryDecode(Message& out) { return codec_.tryDecode(rx_, out); }

    // OS/socket is ready to read (e.g. epoll IN event)
    IoResult onReadable()
    {
        std::array<std::byte, CHUNK_SIZE> buf{};
        // read data from transport to readBuf
        ssize_t n = transport_.recvBytes(buf.data(), buf.size());

        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return {IoStatus::WouldBlock, 0, 0, ""};
            }
            return {IoStatus::Error, 0, errno, "Error reading from transport"};
        }
        else if (n == 0) {
            // connection closed
            close();
            return {IoStatus::Closed, 0, 0, "Connection closed by peer"};
        }
        // append received data to read data buffer
        rx_.append(std::span<const std::byte>(buf.data(), (size_t)n));

        return {IoStatus::Ok, (size_t)n, 0, ""};
    }

    // OS/socket is ready to write (e.g. epoll OUT event)
    IoResult onWritable()
    {
        if (closed_) {
            return {IoStatus::Closed, 0, 0, "Connection is closed"};
        }
        if (tx_.remaining() == 0) {
            // nothing to write
            return {IoStatus::Ok, 0, 0, ""};
        }

        // send data from writeBuf to transport
        ssize_t n = transport_.sendBytes(tx_.data(), tx_.remaining());

        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return {IoStatus::WouldBlock, 0, 0, ""};
            }
            return {IoStatus::Error, 0, errno, "Error writing to transport"};
        }
        else if (n == 0) {
            // connection closed
            close();
            return {IoStatus::Closed, 0, 0, "Connection closed by peer"};
        }
        // consume sent data from write buffer
        tx_.consume((size_t)n);

        return {IoStatus::Ok, (size_t)n, 0, ""};
    }

    bool isClosed() const { return closed_; }
    bool wantsWrite() const { return tx_.remaining() > 0; }
    void close()
    {
        if (!closed_) {
            transport_.disconnect();
            closed_ = true;
        }
    }

private:
    IMessageCodec& codec_;
    IStreamTransport& transport_;

    DataBufferByteQueue rx_;
    DataBufferByteQueue tx_;

    // TODO: consider threading model
    // ThreadSafeQueue<Message> inbox_;
    // ThreadSafeQueue<DataBuffer> outbox_;

    bool closed_{false};
};
