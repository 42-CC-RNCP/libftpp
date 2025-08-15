#include <gtest/gtest.h>
#include <vector>
#include <map>
#include <optional>
#include <variant>
#include <tuple>
#include "data_structures/data_buffer.hpp"

TEST(DataBufferSTL, VectorIntRoundTrip)
{
    DataBuffer buf; std::vector<int> v{1,2,3,4};
    buf << v;
    DataBuffer in; in.putBytes(buf.data(), buf.size());
    std::vector<int> w; in >> w;
    EXPECT_EQ(w, v);
}

TEST(DataBufferSTL, MapStringIntRoundTrip)
{
    DataBuffer buf; std::map<std::string,int> m{{"A",1},{"B",2}};
    buf << m;
    DataBuffer in; in.putBytes(buf.data(), buf.size());
    std::map<std::string,int> n; in >> n;
    EXPECT_EQ(n, m);
}

TEST(DataBufferSTL, OptionalVariantTuple)
{
    DataBuffer buf;
    std::optional<std::string> o = "hi";
    std::variant<int,std::string> v = std::string("xyz");
    std::tuple<int,std::string,double> t{7,"x",2.5};

    buf << o << v << t;
    DataBuffer in; in.putBytes(buf.data(), buf.size());

    std::optional<std::string> o2; std::variant<int,std::string> v2; decltype(t) t2;
    in >> o2 >> v2 >> t2;

    ASSERT_TRUE(o2.has_value()); EXPECT_EQ(*o2, "hi");
    EXPECT_EQ(std::get<std::string>(v2), "xyz");
    EXPECT_EQ(t2, t);
}
