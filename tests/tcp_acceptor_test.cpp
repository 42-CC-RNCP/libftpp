// tests/tcp_acceptor_test.cpp
#include "network/tcp_acceptor.hpp"
#include <chrono>
#include <gtest/gtest.h>
#include <thread>

class TCPAcceptorTest : public ::testing::Test
{
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(TCPAcceptorTest, DefaultConstructorCreatesValidAcceptor)
{
    TCPAcceptor acceptor;
    // Should construct without throwing
}

TEST_F(TCPAcceptorTest, NativeHandleInitiallyInvalid)
{
    TCPAcceptor acceptor;
    EXPECT_EQ(acceptor.nativeHandle(), -1);
}

TEST_F(TCPAcceptorTest, ListenCreatesValidSocket)
{
    TCPAcceptor acceptor;

    // Use a high port number to avoid permission issues
    acceptor.listen(9999);

    EXPECT_GE(acceptor.nativeHandle(), 0);
}

TEST_F(TCPAcceptorTest, ListenOnOccupiedPortThrows)
{
    TCPAcceptor acceptor1;
    acceptor1.listen(9998);

    TCPAcceptor acceptor2;
    EXPECT_THROW(acceptor2.listen(9998), std::runtime_error);
}

TEST_F(TCPAcceptorTest, TryAcceptWithNoConnectionReturnsNull)
{
    TCPAcceptor acceptor;
    acceptor.listen(9997);

    auto transport = acceptor.tryAccept();

    EXPECT_EQ(transport, nullptr);
}

TEST_F(TCPAcceptorTest, DestructorClosesSocket)
{
    int handle = -1;
    {
        TCPAcceptor acceptor;
        acceptor.listen(9996);
        handle = acceptor.nativeHandle();
        EXPECT_GE(handle, 0);
    }
    // acceptor is now destroyed
    // Socket should be closed (difficult to verify directly without OS-specific
    // calls)
}

TEST_F(TCPAcceptorTest, ListenOnZeroPortIsValid)
{
    TCPAcceptor acceptor;

    // Port 0 means OS will assign a free port
    acceptor.listen(0);

    EXPECT_GE(acceptor.nativeHandle(), 0);
}

TEST_F(TCPAcceptorTest, CanListenOnMultiplePorts)
{
    TCPAcceptor acceptor1;
    TCPAcceptor acceptor2;

    acceptor1.listen(9995);
    acceptor2.listen(9994);

    EXPECT_GE(acceptor1.nativeHandle(), 0);
    EXPECT_GE(acceptor2.nativeHandle(), 0);
    EXPECT_NE(acceptor1.nativeHandle(), acceptor2.nativeHandle());
}
