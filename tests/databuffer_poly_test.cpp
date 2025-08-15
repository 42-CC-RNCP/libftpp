#include "data_structures/data_buffer.hpp"
#include "data_structures/poly.hpp"
#include <gtest/gtest.h>
#include <memory>

struct Shape
{
    virtual ~Shape() = default;
};
struct Circle : Shape
{
    float r{};
};
struct Rect : Shape
{
    float w{}, h{};
};

TEST(DataBufferPoly, BasePointerRoundTrip)
{
    register_type<Circle>(
        "Circle",
        /*enc*/
        [](const void* p, DataBuffer& b) {
            b << static_cast<const Circle*>(p)->r;
        },
        /*dec*/
        [](DataBuffer& b, void* p) {
            b >> static_cast<Circle*>(p)->r;
        });
    register_type<Rect>(
        "Rect",
        [](const void* p, DataBuffer& b) {
            auto& r = *static_cast<const Rect*>(p);
            b << r.w << r.h;
        },
        [](DataBuffer& b, void* p) {
            auto& r = *static_cast<Rect*>(p);
            b >> r.w >> r.h;
        });

    std::unique_ptr<Shape> s1 = std::make_unique<Circle>(Circle{3.0f});
    std::unique_ptr<Shape> s2 = std::make_unique<Rect>(Rect{4.0f, 5.0f});

    DataBuffer buf;
    buf << s1 << s2;

    DataBuffer in;
    in.putBytes(buf.data(), buf.size());
    std::unique_ptr<Shape> o1, o2;
    in >> o1 >> o2;

    auto* c = dynamic_cast<Circle*>(o1.get());
    auto* r = dynamic_cast<Rect*>(o2.get());
    ASSERT_TRUE(c && r);
    EXPECT_FLOAT_EQ(c->r, 3.0f);
    EXPECT_FLOAT_EQ(r->w, 4.0f);
    EXPECT_FLOAT_EQ(r->h, 5.0f);
}
