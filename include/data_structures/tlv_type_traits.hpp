#pragma once
#include <cstddef>
#include <cstdint>
#include <type_traits>

template<class> inline constexpr bool dependent_false_v = false;

class ByteIO;

// serialization traits
template <class T, class IO, class = void>
struct is_serializable : std::false_type
{
};

template <class T, class IO>
struct is_serializable<
    T,
    IO,
    std::void_t<
        decltype(serialize(std::declval<const T&>(), std::declval<IO&>())),
        decltype(deserialize(std::declval<IO&>(), std::declval<T&>()))>> :
    std::true_type
{
};

template <class T, class IO>
inline constexpr bool serializable_v = is_serializable<T, IO>::value;

// byte-like object can do bulk copy
template <class T>
inline constexpr bool raw_byte_like_v =
    std::is_same_v<std::remove_cv_t<T>, std::byte>
    || std::is_same_v<std::remove_cv_t<T>, unsigned char>
    || std::is_same_v<std::remove_cv_t<T>, std::uint8_t>
    || std::is_same_v<std::remove_cv_t<T>, char>;
