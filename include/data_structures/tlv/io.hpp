#pragma once
#include <concepts>
#include <cstddef>
#include <span>

namespace tlv
{

// need to be able to compile and return void
template <class T>
concept ByteWriter = requires(T& t, std::span<const std::byte> s) {
    { t.writeBytes(s) } -> std::same_as<void>;
};

template <class T>
concept ByteReader = requires(T& t, std::byte* p, std::size_t n) {
    { t.readExact(p, n) } -> std::same_as<void>;
};

template <class T>
concept ByteIO = ByteWriter<T> && ByteReader<T>;

} // namespace tlv
