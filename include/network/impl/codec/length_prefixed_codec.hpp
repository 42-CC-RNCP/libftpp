// network/impl/codec/length_prefixed_codec.hpp
#pragma once
#include "network/contracts/message_codec.hpp"
#include "utils/endian.hpp"
#include <cstddef>
#include <cstdint>
#include <stdexcept>

class LengthPrefixedCodec : public IMessageCodec
{
public:
    void encode(const Message& msg, ByteQueue& out) override
    {
        const auto& payload = msg.bytes();

        std::uint32_t totalLen =
            kTypeBytes + static_cast<std::uint32_t>(payload.size());

        std::byte lenBytes[4];
        utils::write_endian<std::uint32_t>(
            totalLen, std::span<std::byte>{lenBytes}, std::endian::big);
        out.append(std::span<std::byte>{lenBytes});

        std::byte typeBytes[4];
        utils::write_endian<std::uint32_t>(
            msg.type(), std::span<std::byte>{typeBytes}, std::endian::big);
        out.append(std::span<std::byte>{typeBytes});

        if (!payload.empty()) {
            out.append(std::span<const std::byte>{payload});
        }
    }

    DecodeResult tryDecode(ByteQueue& in, Message& outMsg) override
    {
        if (in.remaining() < kHeaderBytes) {
            return {
                DecodeStatus::NeedMoreData, -1, "Not enough data for header"};
        }
        try {
            decode(in, outMsg);
        }
        catch (const std::exception& e) {
            return {DecodeStatus::Invalid,
                    -1,
                    "Failed to decode: " + std::string(e.what())};
        }
        return {DecodeStatus::Ok, 0, ""};
    }

private:
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

        Message decoded(type);
        decoded.setBytes(std::move(payload));
        outMsg = std::move(decoded);

        in.consume(totalBytes);
    }

    static constexpr std::size_t kLenBytes = 4;
    static constexpr std::size_t kTypeBytes = 4;
    static constexpr std::size_t kHeaderBytes = kLenBytes + kTypeBytes;
};
