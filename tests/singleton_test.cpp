// singleton_test.cpp
#include <atomic>
#include <design_patterns/singleton.hpp>
#include <gtest/gtest.h>
#include <string>
#include <thread>
#include <vector>

struct Probe
{
    int x;
    std::string s;
    static inline std::atomic<int> live{0};
    Probe(int x_, std::string s_) : x(x_), s(std::move(s_)) { ++live; }
    ~Probe() { --live; }
};

using PSingle = Singleton<Probe>;

TEST(SingletonTest, InstanceThrowsWhenNotInstantiated)
{
    PSingle::destroy();

    EXPECT_THROW({ (void)PSingle::instance(); }, std::runtime_error);
}

TEST(SingletonTest, InstantiateOnceSucceeds)
{
    PSingle::destroy();
    PSingle::instantiate(42, "hi");
    auto* p = PSingle::instance();
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->x, 42);
    EXPECT_EQ(p->s, "hi");
    EXPECT_EQ(Probe::live.load(), 1);
}

TEST(SingletonTest, DoubleInstantiateThrows)
{
    PSingle::destroy();
    PSingle::instantiate(1, "a");
    EXPECT_THROW(PSingle::instantiate(2, "b"), std::runtime_error);
    auto* p = PSingle::instance();
    EXPECT_EQ(p->x, 1);
    EXPECT_EQ(p->s, "a");
    EXPECT_EQ(Probe::live.load(), 1);
}

TEST(SingletonTest, DestroyThenReinstantiateWorks)
{
    PSingle::destroy();
    PSingle::instantiate(7, "first");
    EXPECT_EQ(Probe::live.load(), 1);

    PSingle::destroy();
    EXPECT_EQ(Probe::live.load(), 0);

    PSingle::instantiate(9, "second");
    auto* p = PSingle::instance();
    EXPECT_EQ(p->x, 9);
    EXPECT_EQ(p->s, "second");
    EXPECT_EQ(Probe::live.load(), 1);
}

TEST(SingletonTest, ConcurrentInstantiateOnlyOneWins)
{
    PSingle::destroy();

    std::atomic<int> success{0}, failures{0};
    auto worker = [&] {
        try {
            PSingle::instantiate(3, "concurrent");
            success++;
        }
        catch (...) {
            failures++;
        }
    };

    std::vector<std::thread> ts;
    for (int i = 0; i < 16; ++i) {
        ts.emplace_back(worker);
    }
    for (auto& t : ts) {
        t.join();
    }

    EXPECT_EQ(success.load(), 1);
    EXPECT_EQ(failures.load(), 15);
    auto* p = PSingle::instance();
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(p->x, 3);
    EXPECT_EQ(p->s, "concurrent");
    EXPECT_EQ(Probe::live.load(), 1);
}
