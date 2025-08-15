#include "data_structures/data_buffer.hpp"
#include "data_structures/schema.hpp"
#include <gtest/gtest.h>

struct Car
{
    std::uint32_t id{};
    std::string brand;
    float cc{};
};
struct Address
{
    std::string city;
    std::uint32_t zip{};
};
struct PersonV1
{
    std::uint32_t id{};
    std::string name;
    Address addr;
};
struct PersonV2
{
    std::uint32_t id{};
    std::string name;
    Address addr;
    std::string email;
};

TEST(DataBufferSchema, RegisterAndRoundTrip)
{
    register_schema<Car>(
        field<1>(&Car::id), field<2>(&Car::brand), field<3>(&Car::cc));
    register_schema<Address>(field<1>(&Address::city), field<2>(&Address::zip));
    register_schema<PersonV1>(field<1>(&PersonV1::id),
                              field<2>(&PersonV1::name),
                              field<3>(&PersonV1::addr));

    Car c{7, "Audi", 5.2f};
    DataBuffer buf;
    buf << c;

    DataBuffer in;
    in.putBytes(buf.data(), buf.size());
    Car d;
    in >> d;

    EXPECT_EQ(d.id, 7u);
    EXPECT_EQ(d.brand, "Audi");
    EXPECT_FLOAT_EQ(d.cc, 5.2f);
}

TEST(DataBufferSchema, VersioningSkipUnknown)
{
    register_schema<PersonV2>(field<1>(&PersonV2::id),
                              field<2>(&PersonV2::name),
                              field<3>(&PersonV2::addr),
                              field<4>(&PersonV2::email));

    PersonV2 v2{42, "Alice", {"Roma", 12345}, "a@x.com"};
    DataBuffer pkt;
    pkt << v2;

    register_schema<PersonV1>(field<1>(&PersonV1::id),
                              field<2>(&PersonV1::name),
                              field<3>(&PersonV1::addr));

    DataBuffer in;
    in.putBytes(pkt.data(), pkt.size());
    PersonV1 v1{};
    in >> v1;

    EXPECT_EQ(v1.id, 42u);
    EXPECT_EQ(v1.name, "Alice");
    EXPECT_EQ(v1.addr.city, "Roma");
    EXPECT_EQ(v1.addr.zip, 12345u);
}
