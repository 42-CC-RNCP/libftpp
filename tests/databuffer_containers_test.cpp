#include "data_buffer.hpp"
#include "tlv.hpp"
#include <array>
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace tlv
{
template <class IO, class T> void serialize(const std::vector<T>& xs, IO& out)
{
    detail::write_varuint(out, static_cast<std::uint64_t>(xs.size()));
    for (const auto& e : xs) {
        write_value(out, e);
    }
}
template <class IO, class T> void deserialize(IO& in, std::vector<T>& xs)
{
    std::size_t n = static_cast<std::size_t>(detail::read_varuint(in));
    if constexpr (requires { in.limits().max_elements; }) {
        if (n > in.limits().max_elements) {
            throw std::runtime_error("too many elements");
        }
    }
    xs.clear();
    xs.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        T e{};
        read_value(in, e);
        xs.push_back(std::move(e));
    }
}

template <class IO, class T, std::size_t N>
void serialize(const std::array<T, N>& xs, IO& out)
{
    for (const auto& e : xs) {
        write_value(out, e);
    }
}
template <class IO, class T, std::size_t N>
void deserialize(IO& in, std::array<T, N>& xs)
{
    for (auto& e : xs) {
        read_value(in, e);
    }
}
} // namespace tlv

TEST(DataBufferContainers, Vector_Int_Roundtrip)
{
    DataBuffer buf;
    std::vector<int> v_in{1, -2, 3, -4, 123456};

    buf << v_in;

    std::vector<int> v_out;
    buf >> v_out;
    EXPECT_EQ(v_out, v_in);
}

TEST(DataBufferContainers, Vector_String_Roundtrip)
{
    DataBuffer buf;
    std::vector<std::string> v_in{"", "a", "hello", "世界"};

    buf << v_in;

    std::vector<std::string> v_out;
    buf >> v_out;

    ASSERT_EQ(v_out.size(), v_in.size());
    for (size_t i = 0; i < v_in.size(); ++i) {
        EXPECT_EQ(v_out[i], v_in[i]);
    }
}

TEST(DataBufferContainers, Array_Fixed_Roundtrip)
{
    DataBuffer buf;
    std::array<uint32_t, 5> a_in{{10u, 11u, 12u, 13u, 14u}};

    buf << a_in;

    std::array<uint32_t, 5> a_out{};
    buf >> a_out;

    EXPECT_EQ(a_out, a_in);
}

TEST(DataBufferContainers, Nested_Vector_Vector)
{
    DataBuffer buf;
    std::vector<std::vector<uint32_t>> x{{1, 2, 3}, {}, {42, 1000, 0}};

    buf << x;

    std::vector<std::vector<uint32_t>> y;
    buf >> y;

    EXPECT_EQ(y, x);
}
