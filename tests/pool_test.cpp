#include <gtest/gtest.h>
#include <data_structures/pool.hpp>

struct Dummy {
    static inline size_t ctor = 0;
    static inline size_t dtor = 0;
    static inline size_t moveCtor = 0;
    static inline size_t copyCtor = 0;

    int a{};
    std::string s;

    Dummy() { ++ctor; }
    Dummy(int aa, std::string ss) : a(aa), s(std::move(ss)) { ++ctor; }

    Dummy(const Dummy& o) : a(o.a), s(o.s) { ++ctor; ++copyCtor; }
    Dummy(Dummy&& o) noexcept : a(o.a), s(std::move(o.s)) { ++ctor; ++moveCtor; }

    ~Dummy() { ++dtor; }
};

TEST(PoolTest, CtorDtor_Exact_WithEmplace) {
    Dummy::ctor = Dummy::dtor = Dummy::moveCtor = Dummy::copyCtor = 0;

    {
        Pool<Dummy> pool(3);
        auto h1 = pool.acquire(1, "x");
        auto h2 = pool.acquire(2, "y");
        auto h3 = pool.acquire(3, "z");

        EXPECT_EQ(Dummy::ctor, 3);
        EXPECT_EQ(Dummy::dtor, 0);
    }
    EXPECT_EQ(Dummy::dtor, 3);
}

TEST(PoolTest, ReuseSameAddress) {
    Dummy::ctor = Dummy::dtor = 0;
    Pool<Dummy> pool(1);
    Dummy* p = nullptr;

    {
        auto h = pool.acquire(10, "a");
        p = &*h;
    }
    EXPECT_EQ(Dummy::dtor, 1u);

    auto h2 = pool.acquire(20, "b");
    EXPECT_EQ(&*h2, p);
}

TEST(PoolTest, AcquireAndDereference) {
    Pool<Dummy> pool(3);
    {
        auto obj = pool.acquire(42, "hello");
        EXPECT_EQ(obj->a, 42);
        EXPECT_EQ(obj->s, "hello");
        EXPECT_EQ((*obj).a, 42);
        EXPECT_EQ((*obj).s, "hello");
    }  // test automatic release via destructor
}

TEST(PoolTest, AcquireAllAndThrow) {
    Pool<Dummy> pool(1);
    auto obj = pool.acquire(1, "test");
    EXPECT_THROW(pool.acquire(2, "fail"), std::runtime_error);
}

TEST(PoolTest, ResizeCreatesObjects) {
    Pool<Dummy> pool(0);
    pool.resize(5);
    for (int i = 0; i < 5; ++i) {
        auto obj = pool.acquire(i, std::to_string(i));
        EXPECT_EQ(obj->a, i);
        EXPECT_EQ(obj->s, std::to_string(i));
    }
}

TEST(PoolTest, MoveConstructorTransfersOwnership) {
    Pool<Dummy> pool(1);
    auto obj1 = pool.acquire(1, "x");
    Dummy* rawPtr = obj1.operator->();
    Pool<Dummy>::Object obj2 = std::move(obj1);
    EXPECT_EQ(obj2->a, 1);
    EXPECT_EQ(obj2->s, "x");
    EXPECT_EQ(obj2.operator->(), rawPtr);
}

TEST(PoolTest, MoveAssignmentTransfersOwnership) {
    Pool<Dummy> pool(2);
    auto obj1 = pool.acquire(5, "A");
    auto obj2 = pool.acquire(6, "B");
    Dummy* raw1 = obj1.operator->();
    obj2 = std::move(obj1);
    EXPECT_EQ(obj2->a, 5);
    EXPECT_EQ(obj2->s, "A");
    EXPECT_EQ(obj2.operator->(), raw1);
}

TEST(PoolTest, ReleaseAndReuse) {
    Pool<Dummy> pool(1);
    Dummy* original = nullptr;
    {
        auto obj = pool.acquire(10, "abc");
        original = obj.operator->();
    }
    auto obj2 = pool.acquire(20, "xyz");
    EXPECT_EQ(obj2.operator->(), original);
    EXPECT_EQ(obj2->a, 20);
    EXPECT_EQ(obj2->s, "xyz");
}
