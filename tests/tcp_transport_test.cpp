// tests/tcp_transport_test.cpp
#include "network/endpoint.hpp"
#include "network/tcp_transport.hpp"
#include <gtest/gtest.h>

class TCPTransportTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

TEST_F(TCPTransportTest, DefaultConstructorCreatesDisconnectedTransport)
{
    TCPTransport transport;

    EXPECT_FALSE(transport.isConnected());
}

TEST_F(TCPTransportTest, ConstructorWithValidFdMarksConnected)
{
    int mockFd = 10; // Arbitrary positive value
    TCPTransport transport(mockFd);

    // This will be true because fd >= 0
    EXPECT_TRUE(transport.isConnected());
}

TEST_F(TCPTransportTest, ConstructorWithNegativeFdMarksDisconnected)
{
    TCPTransport transport(-1);

    EXPECT_FALSE(transport.isConnected());
}

TEST_F(TCPTransportTest, DisconnectMarksTransportAsDisconnected)
{
    // Create with valid fd
    TCPTransport transport(10);
    EXPECT_TRUE(transport.isConnected());

    transport.disconnect();

    EXPECT_FALSE(transport.isConnected());
}

TEST_F(TCPTransportTest, ConnectToInvalidAddressThrows)
{
    TCPTransport transport;
    Endpoint ep{"999.999.999.999", 9999};

    EXPECT_THROW(transport.connect(ep), std::runtime_error);
}

TEST_F(TCPTransportTest, RecvBytesThrowsWhenNotConnected)
{
    TCPTransport transport;
    std::byte buffer[10];

    EXPECT_THROW(transport.recvBytes(buffer, sizeof(buffer)),
                 std::runtime_error);
}

TEST_F(TCPTransportTest, SendBytesThrowsWhenNotConnected)
{
    TCPTransport transport;
    std::byte data[] = {std::byte{0x01}};

    EXPECT_THROW(transport.sendBytes(data, sizeof(data)), std::runtime_error);
}

TEST_F(TCPTransportTest, MultipleDisconnectsAreSafe)
{
    TCPTransport transport(10);

    transport.disconnect();
    transport.disconnect();
    transport.disconnect();

    EXPECT_FALSE(transport.isConnected());
}
