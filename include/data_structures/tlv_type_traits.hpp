#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <forward_list>
#include <iterator>
#include <list>
#include <map>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

template <class> inline constexpr bool dependent_false_v = false;

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

// basic string / array
template <class T> struct is_basic_string : std::false_type
{
};
template <class Ch, class Tr, class Al>
struct is_basic_string<std::basic_string<Ch, Tr, Al>> : std::true_type
{
};

template <class T> struct is_std_array : std::false_type
{
};

template <class T, std::size_t N>
struct is_std_array<std::array<T, N>> : std::true_type
{
};

// detect container shape likes
template <class T>
concept has_value_type = requires { typename T::value_type; };

template <class T>
concept has_key_type = requires { typename T::key_type; };

template <class T>
concept has_mapped_type = requires { typename T::mapped_type; };

template <class T, class = void> struct has_begin_end : std::false_type
{
};
template <class T>
struct has_begin_end<T,
                     std::void_t<decltype(std::begin(std::declval<T&>())),
                                 decltype(std::end(std::declval<T&>()))>> :
    std::true_type
{
};

template <class T>
inline constexpr bool is_basic_string_v = is_basic_string<T>::value;
template <class T>
inline constexpr bool is_std_array_v = is_std_array<T>::value;
template <class T>
inline constexpr bool has_begin_end_v = has_begin_end<T>::value;

template <class T>
inline constexpr bool is_map_like_v = has_key_type<T> && has_mapped_type<T>;

template <class T>
inline constexpr bool is_set_like_v = has_key_type<T> && !has_mapped_type<T>;

template <class T>
concept is_container_like = requires {
    typename T::value_type;
} && has_begin_end_v<T> && !is_basic_string_v<std::remove_cvref_t<T>>;
