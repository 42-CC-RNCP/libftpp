#pragma once
#include "message_codec.hpp"
#include "utils/endian.hpp"
#include <cstddef>
#include <cstdint>
#include <exception>
#include <stdexcept>

// A simple length-prefixed codec implementation.
// Message format:
// [4 bytes length][4 bytes type][payload]
class LengthPrefixedCodec : public IMessageCodec
{
public:
    void encode(const Message& msg, ByteQueue& out) override
    {
        // calculate total length = type bytes + payload bytes
        std::uint32_t totalLen =
            kTypeBytes + static_cast<std::uint32_t>(msg.payload().remaining());

        // write length (big-endian)
        std::byte lenBytes[4];
        utils::write_endian<std::uint32_t>(
            totalLen, {lenBytes, 4}, std::endian::big);
        out.append({lenBytes, 4});

        // write type (big-endian)
        std::byte typeBytes[4];
        utils::write_endian<std::uint32_t>(
            msg.type(), {typeBytes, 4}, std::endian::big);
        out.append({typeBytes, 4});

        // write payload
        if (msg.payload().remaining() > 0) {
            out.append(msg.payload().peek());
        }
    }

    void decode(ByteQueue& in, Message& outMsg)
    {
        auto view = in.peek();
        std::uint32_t len = utils::read_endian<std::uint32_t>(
            view.subspan(0, kLenBytes), std::endian::big);

        if (len < kTypeBytes) {
            throw std::runtime_error("Invalid message length");
        }

        std::size_t totalBytes = kLenBytes + len;
        if (in.remaining() < totalBytes) {
            throw std::runtime_error("Incomplete message");
        }

        std::uint32_t type = utils::read_endian<std::uint32_t>(
            view.subspan(kLenBytes, kTypeBytes), std::endian::big);

        std::size_t payloadSize = len - kTypeBytes;
        std::vector<std::byte> payload;
        if (payloadSize > 0) {
            auto payloadView = view.subspan(kHeaderBytes, payloadSize);
            payload.assign(payloadView.begin(), payloadView.end());
        }

        outMsg.type() = type;
        outMsg.payload(std::move(payload));
        in.consume(totalBytes);
    }

    DecodeResult tryDecode(ByteQueue& in, Message& outMsg) override
    {
        // strong exception guarantee
        if (in.remaining() < kHeaderBytes) {
            return {
                DecodeStatus::NeedMoreData, -1, "Not enough data for header"};
        }
        try {
            decode(in, outMsg);
        }
        catch (std::exception& e) {
            return {DecodeStatus::Invalid,
                    -1,
                    "Failed to decode message: " + std::string(e.what())};
        }
        catch (...) {
            return {DecodeStatus::Invalid,
                    -1,
                    "Failed to decode message: unknown error"};
        }
        return {DecodeStatus::Ok, 0, ""};
    }

private:
    static constexpr std::size_t kLenBytes = 4;
    static constexpr std::size_t kTypeBytes = 4;
    static constexpr std::size_t kHeaderBytes = kLenBytes + kTypeBytes;
};
