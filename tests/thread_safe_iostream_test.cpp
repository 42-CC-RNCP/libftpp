// tests/thread_safe_iostream_test.cpp
#include <atomic>
#include <gtest/gtest.h>
#include <iostream/thread_safe_iostream.hpp>
#include <iostream>
#include <istream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

// capure threadSafeCout for tests
struct CoutCapture
{
    std::ostringstream oss_;
    std::streambuf* old_{nullptr};
    CoutCapture() { old_ = std::cout.rdbuf(oss_.rdbuf()); }
    ~CoutCapture() { std::cout.rdbuf(old_); }
    std::string str() const { return oss_.str(); }
};

struct CinRedirect
{
    std::istringstream iss_;
    std::streambuf* old_{nullptr};
    explicit CinRedirect(const std::string& data) : iss_(data)
    {
        old_ = std::cin.rdbuf(iss_.rdbuf());
    }
    ~CinRedirect() { std::cin.rdbuf(old_); }
};

// utility: split string into lines by '\n'
static std::vector<std::string> split_lines(const std::string& s)
{
    std::vector<std::string> lines;
    std::string cur;
    std::istringstream is(s);
    while (std::getline(is, cur)) {
        lines.push_back(cur);
    }
    return lines;
}

// Global thread-local instance for tests
thread_local ThreadSafeIOStream threadSafeCout;

TEST(ThreadSafeIOStream, EachThreadHasIndependentPrefix)
{
    CoutCapture cap;

    std::thread t1([] {
        threadSafeCout.setPrefix("[T1] ");
        threadSafeCout << "hello\n";
    });
    std::thread t2([] {
        threadSafeCout.setPrefix("[T2] ");
        threadSafeCout << "world\n";
    });
    t1.join();
    t2.join();

    auto lines = split_lines(cap.str());
    ASSERT_EQ(lines.size(), 2u);
    bool ok1 = false, ok2 = false;
    for (auto& l : lines) {
        if (l.rfind("[T1] ", 0) == 0 && l.find("hello") != std::string::npos) {
            ok1 = true;
        }
        if (l.rfind("[T2] ", 0) == 0 && l.find("world") != std::string::npos) {
            ok2 = true;
        }
    }
    EXPECT_TRUE(ok1);
    EXPECT_TRUE(ok2);
}

TEST(ThreadSafeIOStream, WholeLineIsAtomic_NoInterleaving)
{
    CoutCapture cap;

    const int N = 2000;
    std::thread t1([&] {
        threadSafeCout.setPrefix("[A] ");
        for (int i = 0; i < N; ++i) {
            threadSafeCout << "line-" << i << '\n';
        }
    });
    std::thread t2([&] {
        threadSafeCout.setPrefix("[B] ");
        for (int i = 0; i < N; ++i) {
            threadSafeCout << "line-" << i << '\n';
        }
    });
    t1.join();
    t2.join();

    auto lines = split_lines(cap.str());
    ASSERT_EQ(lines.size(), static_cast<size_t>(2 * N));

    for (auto& l : lines) {
        ASSERT_TRUE(l.rfind("[A] ", 0) == 0 || l.rfind("[B] ", 0) == 0) << l;
        EXPECT_FALSE(l.find("[A] ") != std::string::npos
                     && l.find("[B] ") != std::string::npos)
            << l;
    }
}

TEST(ThreadSafeIOStream, MultipleNewlinesInOneWriteEmitMultiplePrefixedLines)
{
    CoutCapture cap;
    std::thread t([] {
        threadSafeCout.setPrefix("[M] ");
        threadSafeCout << "L1\nL2\nL3\n";
    });
    t.join();

    auto lines = split_lines(cap.str());
    ASSERT_EQ(lines.size(), 3u);
    EXPECT_EQ(lines[0], "[M] L1");
    EXPECT_EQ(lines[1], "[M] L2");
    EXPECT_EQ(lines[2], "[M] L3");
}

TEST(ThreadSafeIOStream, StdEndlForcesFlushAsNewline)
{
    CoutCapture cap;
    std::thread t([] {
        threadSafeCout.setPrefix("[E] ");
        threadSafeCout << "X" << std::endl;
    });
    t.join();

    auto lines = split_lines(cap.str());
    ASSERT_EQ(lines.size(), 1u);
    EXPECT_EQ(lines[0], "[E] X");
}

TEST(ThreadSafeIOStream, PartialLineIsBuffered_NotPrintedUntilNewline)
{
    CoutCapture cap;
    threadSafeCout.setPrefix("[P] ");
    threadSafeCout << "partial";

    EXPECT_TRUE(cap.str().empty()) << "Should not flush partial line";

    threadSafeCout << "\n";
    auto lines = split_lines(cap.str());
    ASSERT_EQ(lines.size(), 1u);
    EXPECT_EQ(lines[0], "[P] partial");
}

TEST(ThreadSafeIOStream, PromptIsSerializedAndPrefixed)
{
    CinRedirect cinRedir("42\n7\n");
    CoutCapture cap;

    std::atomic<int> a{0}, b{0};

    auto askA = [&] {
        threadSafeCout.setPrefix("[A] ");
        int x = 0;
        threadSafeCout.prompt("Enter A: ", x);
        a = x;
    };
    auto askB = [&] {
        threadSafeCout.setPrefix("[B] ");
        int y = 0;
        threadSafeCout.prompt("Enter B: ", y);
        b = y;
    };

    std::thread t1(askA), t2(askB);
    t1.join();
    t2.join();

    int v1 = a.load(), v2 = b.load();
    EXPECT_EQ(v1 + v2, 49);

    auto out = split_lines(cap.str());
    ASSERT_EQ(out.size(), 2u);
    bool okA = false, okB = false;
    for (auto& l : out) {
        if (l == "[A] Enter A: ") {
            okA = true;
        }
        if (l == "[B] Enter B: ") {
            okB = true;
        }
    }
    EXPECT_TRUE(okA);
    EXPECT_TRUE(okB);
}

TEST(ThreadSafeIOStream, ThreadLocalInstanceExistsPerThread)
{
    const void* addr_main = static_cast<const void*>(&threadSafeCout);

    const void* addr_t = nullptr;
    std::thread t([&] {
        addr_t = static_cast<const void*>(&threadSafeCout);
    });
    t.join();

    EXPECT_NE(addr_main, addr_t);
}

