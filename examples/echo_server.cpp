#include "network/message.hpp"
#include "network/server.hpp"
// #include "network/tcp_transport.hpp"
#include "network/length_prefixed_codec.hpp"
#include "network/tcp_acceptor.hpp"

int main()
{
    Message::Type echoType = 1; // Define a custom message type for echo
    Server server(std::make_unique<TCPAcceptor>(), []() {
        return std::make_unique<LengthPrefixedCodec>();
    });

    server.defineAction(echoType, [](PeerHandle& peer, const Message& msg) {
        // Echo the received message back to the sender
        Message reply = msg; 
        peer.send(std::move(reply));
    });

    // server.start(8080);

    // while (true) {
    //     server.update();
    // }
}
