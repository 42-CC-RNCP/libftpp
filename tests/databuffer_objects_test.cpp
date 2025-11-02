#include <data_structures/data_buffer.hpp>
#include <data_structures/tlv.hpp>
#include <data_structures/tlv_adapters.hpp>
#include <gtest/gtest.h>
#include <string>

struct Inner
{
    int score{};
    std::string name;
};
struct Outer
{
    std::uint64_t id{};
    Inner inner;
};
struct Base
{
    int b{};
};
struct Derived : Base
{
    double d{};
};

namespace tlv
{
template <class IO> void serialize(const Inner& x, IO& out)
{
    write_value(out, x.score);
    write_value(out, x.name);
}
template <class IO> void deserialize(IO& in, Inner& x)
{
    read_value(in, x.score);
    read_value(in, x.name);
}

template <class IO> void serialize(const Outer& x, IO& out)
{
    write_value(out, x.id);
    write_value(out, x.inner);
}
template <class IO> void deserialize(IO& in, Outer& x)
{
    read_value(in, x.id);
    read_value(in, x.inner);
}

template <class IO> void serialize(const Base& x, IO& out)
{
    write_value(out, x.b);
}
template <class IO> void deserialize(IO& in, Base& x)
{
    read_value(in, x.b);
}

template <class IO> void serialize(const Derived& x, IO& out)
{
    serialize(static_cast<const Base&>(x), out);
    write_value(out, x.d);
}
template <class IO> void deserialize(IO& in, Derived& x)
{
    deserialize(in, static_cast<Base&>(x));
    read_value(in, x.d);
}
} // namespace tlv

TEST(DataBufferObjects, Nested_Object_Roundtrip)
{
    DataBuffer buf;

    Outer in;
    in.id = 123456789u;
    in.inner.score = 99;
    in.inner.name = "tester";
    buf << in;

    Outer out{};
    buf >> out;

    EXPECT_EQ(out.id, in.id);
    EXPECT_EQ(out.inner.score, in.inner.score);
    EXPECT_EQ(out.inner.name, in.inner.name);
}

TEST(DataBufferObjects, Base_Derived_Roundtrip)
{
    DataBuffer buf;

    Derived in;
    in.b = -7;
    in.d = 6.5;
    buf << in;

    Derived out{};
    buf >> out;

    EXPECT_EQ(out.b, in.b);
    EXPECT_DOUBLE_EQ(out.d, in.d);
}

TEST(DataBufferObjects, Read_Derived_From_Base_Payload_ShouldThrow)
{
    DataBuffer buf;
    Base base_in{42};
    buf << base_in;

    Derived want{};
    EXPECT_THROW((buf >> want), std::runtime_error);
}
