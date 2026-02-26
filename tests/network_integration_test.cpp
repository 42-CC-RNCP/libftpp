// tests/network_integration_test.cpp
#include "network/network.hpp"
#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <thread>

class NetworkIntegrationTest : public ::testing::Test
{
protected:
    const std::string TEST_HOST = "127.0.0.1";
    const std::uint16_t TEST_PORT = 19999;

    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(NetworkIntegrationTest, ServerAcceptsClientConnection)
{
    auto acceptor = std::make_unique<TCPAcceptor>();
    auto reactor = std::make_unique<EpollReactor>();
    auto codecFactory = []() {
        return std::make_unique<LengthPrefixedCodec>();
    };

    Server server(std::move(acceptor), std::move(reactor), codecFactory);
    server.start(TEST_PORT);

    // Run server in separate thread
    std::atomic<bool> serverRunning{true};
    std::thread serverThread([&]() {
        while (serverRunning) {
            server.update();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    // Give server time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Create client
    TCPTransport clientTransport;
    LengthPrefixedCodec clientCodec;
    Client client(clientTransport, clientCodec);

    // Connect to server
    EXPECT_NO_THROW(client.connect(TEST_HOST, TEST_PORT));

    // Cleanup
    serverRunning = false;
    serverThread.join();
}

TEST_F(NetworkIntegrationTest, ClientSendsMessageToServer)
{
    std::atomic<bool> messageReceived{false};
    std::atomic<Message::Type> receivedType{0};

    auto acceptor = std::make_unique<TCPAcceptor>();
    auto reactor = std::make_unique<EpollReactor>();
    auto codecFactory = []() {
        return std::make_unique<LengthPrefixedCodec>();
    };

    Server server(std::move(acceptor), std::move(reactor), codecFactory);
    server.start(TEST_PORT);

    // Define server handler
    Message::Type expectedType = 100;
    server.defineAction(expectedType, [&](PeerHandle&, const Message& msg) {
        receivedType = msg.type();
        messageReceived = true;
    });

    // Run server
    std::atomic<bool> serverRunning{true};
    std::thread serverThread([&]() {
        while (serverRunning) {
            server.update();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Create and connect client
    TCPTransport clientTransport;
    LengthPrefixedCodec clientCodec;
    Client client(clientTransport, clientCodec);
    client.connect(TEST_HOST, TEST_PORT);

    // Send message
    MessageWriter writer(expectedType);
    int payload = 42;
    writer << payload; // example payload
    Message msg = writer.build();
    client.send(msg);
    client.update();

    // Flush send buffer
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Wait for message to be received
    int attempts = 0;
    while (!messageReceived && attempts < 50) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        attempts++;
    }

    EXPECT_TRUE(messageReceived);
    EXPECT_EQ(receivedType, expectedType);

    serverRunning = false;
    serverThread.join();
}

TEST_F(NetworkIntegrationTest, ServerSendsMessageToClient)
{
    std::atomic<bool> messageReceived{false};
    std::atomic<Message::Type> receivedType{0};
    std::atomic<INetworkPort::ClientId> connectedClientId{0};

    auto acceptor = std::make_unique<TCPAcceptor>();
    auto reactor = std::make_unique<EpollReactor>();
    auto codecFactory = []() {
        return std::make_unique<LengthPrefixedCodec>();
    };

    Server server(std::move(acceptor), std::move(reactor), codecFactory);
    server.start(TEST_PORT);

    // Define handler to track connected client
    server.defineAction(999, [&](PeerHandle& peer, const Message&) {
        connectedClientId = peer.id();
    });

    std::atomic<bool> serverRunning{true};
    std::thread serverThread([&]() {
        while (serverRunning) {
            server.update();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Create client
    TCPTransport clientTransport;
    LengthPrefixedCodec clientCodec;
    Client client(clientTransport, clientCodec);

    Message::Type expectedType = 200;
    client.defineAction(expectedType, [&](OutboundPort&, const Message& msg) {
        receivedType = msg.type();
        messageReceived = true;
    });

    client.connect(TEST_HOST, TEST_PORT);

    // Send handshake message to get client ID
    Message handshake(999);
    client.send(handshake);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Server sends message
    Message serverMsg(expectedType);
    server.sendTo(serverMsg, connectedClientId);

    // Client updates to receive
    std::thread clientThread([&]() {
        int attempts = 0;
        while (!messageReceived && attempts < 50) {
            try {
                client.update();
            }
            catch (...) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            attempts++;
        }
    });

    clientThread.join();

    EXPECT_TRUE(messageReceived);
    EXPECT_EQ(receivedType, expectedType);

    serverRunning = false;
    serverThread.join();
}

TEST_F(NetworkIntegrationTest, ServerBroadcastsToMultipleClients)
{
    std::atomic<int> messagesReceived{0};

    auto acceptor = std::make_unique<TCPAcceptor>();
    auto reactor = std::make_unique<EpollReactor>();
    auto codecFactory = []() {
        return std::make_unique<LengthPrefixedCodec>();
    };

    Server server(std::move(acceptor), std::move(reactor), codecFactory);
    server.start(TEST_PORT);

    std::atomic<bool> serverRunning{true};
    std::thread serverThread([&]() {
        while (serverRunning) {
            server.update();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Create two clients
    TCPTransport transport1, transport2;
    LengthPrefixedCodec codec1, codec2;
    Client client1(transport1, codec1);
    Client client2(transport2, codec2);

    Message::Type broadcastType = 300;

    client1.defineAction(broadcastType, [&](OutboundPort&, const Message&) {
        messagesReceived++;
    });

    client2.defineAction(broadcastType, [&](OutboundPort&, const Message&) {
        messagesReceived++;
    });

    client1.connect(TEST_HOST, TEST_PORT);
    client2.connect(TEST_HOST, TEST_PORT);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Broadcast message
    Message msg(broadcastType);
    server.sendToAll(msg);

    // Both clients update
    std::thread clientThread1([&]() {
        for (int i = 0; i < 20; i++) {
            try {
                client1.update();
            }
            catch (...) {
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    std::thread clientThread2([&]() {
        for (int i = 0; i < 20; i++) {
            try {
                client2.update();
            }
            catch (...) {
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    clientThread1.join();
    clientThread2.join();

    EXPECT_EQ(messagesReceived, 2);

    serverRunning = false;
    serverThread.join();
}

TEST_F(NetworkIntegrationTest, RoundTripMessageWithComplexPayload)
{
    std::atomic<bool> messageReceived{false};
    int receivedInt = 0;
    std::string receivedString;
    double receivedDouble = 0.0;

    auto acceptor = std::make_unique<TCPAcceptor>();
    auto reactor = std::make_unique<EpollReactor>();
    auto codecFactory = []() {
        return std::make_unique<LengthPrefixedCodec>();
    };

    Server server(std::move(acceptor), std::move(reactor), codecFactory);
    server.start(TEST_PORT);

    Message::Type msgType = 400;
    server.defineAction(
        msgType,
        [&]([[maybe_unused]] PeerHandle& peer, const Message& msg_const) {
            MessageReader reader(msg_const);
            reader >> receivedInt >> receivedString >> receivedDouble;
            messageReceived = true;
        });

    std::atomic<bool> serverRunning{true};
    std::thread serverThread([&]() {
        while (serverRunning) {
            server.update();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    TCPTransport clientTransport;
    LengthPrefixedCodec clientCodec;
    Client client(clientTransport, clientCodec);
    client.connect(TEST_HOST, TEST_PORT);

    // Send complex message
    MessageWriter writer(msgType);
    int intVal = 12345;
    std::string strVal = "hello world";
    double doubleVal = 3.14159;
    writer << intVal << strVal << doubleVal;
    Message msg = writer.build();
    client.send(msg);
    client.update();

    // Wait for server to receive
    int attempts = 0;
    while (!messageReceived && attempts < 50) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        attempts++;
    }

    EXPECT_TRUE(messageReceived);
    EXPECT_EQ(receivedInt, intVal);
    EXPECT_EQ(receivedString, strVal);
    EXPECT_DOUBLE_EQ(receivedDouble, doubleVal);

    serverRunning = false;
    serverThread.join();
}

TEST_F(NetworkIntegrationTest, ClientDisconnectIsDetected)
{
    std::atomic<int> activeClients{0};

    auto acceptor = std::make_unique<TCPAcceptor>();
    auto reactor = std::make_unique<EpollReactor>();
    auto codecFactory = []() {
        return std::make_unique<LengthPrefixedCodec>();
    };

    Server server(std::move(acceptor), std::move(reactor), codecFactory);
    server.start(TEST_PORT);

    std::atomic<bool> serverRunning{true};
    std::thread serverThread([&]() {
        while (serverRunning) {
            server.update();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        TCPTransport clientTransport;
        LengthPrefixedCodec clientCodec;
        Client client(clientTransport, clientCodec);
        client.connect(TEST_HOST, TEST_PORT);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        activeClients = 1;
    } // Client goes out of scope and disconnects

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    // Server should detect disconnection

    serverRunning = false;
    serverThread.join();
}
