#pragma once
#include "acceptor.hpp"
#include "connection.hpp"
#include "dispatcher.hpp"
#include "message.hpp"
#include "network_port.hpp"
#include "peer_handle.hpp"
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

// a orchestrator for multiple clients, not responsible for low-level details
// inherits INetworkPort to provide sendTo and sendToAll functionalities

using ServerDispatcher = DispatcherT<PeerHandle>;
using ServerHandler = std::function<void(PeerHandle&, const Message&)>;

class Server : public INetworkPort
{
    struct Session
    {
        ClientId id;
        PeerHandle handle;
        std::unique_ptr<IStreamTransport> transport;
        std::unique_ptr<IMessageCodec> codec;
        Connection conn;

        Session(ClientId id_,
                INetworkPort* port,
                std::unique_ptr<IStreamTransport> t,
                std::unique_ptr<IMessageCodec> c) :
            id(id_),
            handle(id_, port),
            transport(std::move(t)),
            codec(std::move(c)),
            conn(*transport, *codec)
        {
        }
    };

public:
    Server(std::unique_ptr<IAcceptor> acceptor,
           std::function<std::unique_ptr<IMessageCodec>()> codecFactory) :
        acceptor_(std::move(acceptor)), codecFactory_(std::move(codecFactory))
    {
    }

    void start(const size_t& p_port)
    {
        acceptor_->listen(static_cast<std::uint16_t>(p_port));
    }

    void defineAction(const Message::Type& t, ServerHandler h)
    {
        dispatcher_.defineRawAction(t, std::move(h));
    }

    void sendTo(const Message& message, ClientId clientID) override
    {
        auto it = sessions_.find(clientID);

        if (it != sessions_.end()) {
            it->second->conn.queue(message);
        }
    }

    void sendToAll(const Message& message) override
    {
        for (auto& [id, session] : sessions_) {
            session->conn.queue(message);
        }
    }

    void sendToArray(const Message& message, std::vector<ClientId> clientIDs)
    {
        for (auto& id : clientIDs) {
            sendTo(message, id);
        }
    }

    void disconnect(ClientId clientID) override { sessions_.erase(clientID); }

    void update()
    {
        _acceptNewConnections();

        for (auto& [id, session] : sessions_) {
            _processSession(*session);
        }

        // erase pending closes after iteration
        for (ClientId id : pendingClose_) {
            auto it = sessions_.find(id);
            if (it != sessions_.end()) {
                it->second->transport->disconnect();
                sessions_.erase(it);
            }
        }
        pendingClose_.clear();
    }

private:
    void _acceptNewConnections()
    {
        while (true) {
            auto t = acceptor_->tryAccept();

            if (!t) {
                break; // No more pending connections
            }

            ClientId id = nextId++;
            auto codec = codecFactory_();
            auto session = std::make_unique<Session>(
                id, this, std::move(t), std::move(codec));
            sessions_.emplace(id, std::move(session));
        }
    }

    void _closeLater(ClientId id) { pendingClose_.push_back(id); }

    void _processSession(Session& s)
    {
        // process incoming messages
        auto io = s.conn.onReadable();

        if (io.status != Connection::IoStatus::Ok) {
            _closeLater(s.id);
            return;
        }

        // decode loop
        while (true) {
            Message msg(0);
            auto dr = s.conn.tryDecode(msg);

            if (dr.status == DecodeStatus::Ok) {
                dispatcher_.dispatch(s.handle, msg);

                // peer handle asked to close the connection
                if (s.conn.isClosed()) {
                    _closeLater(s.id);
                    return;
                }
            }
            else if (dr.status == DecodeStatus::NeedMoreData) {
                break; // wait for more data
            }
            else { // Invalid message
                _closeLater(s.id);
                return;
            }
        }

        // process outgoing messages
        if (s.conn.wantsWrite()) {
            auto wo = s.conn.onWritable();

            if (wo.status != Connection::IoStatus::Ok) {
                _closeLater(s.id);
                return;
            }
        }
    }

private:
    std::unique_ptr<IAcceptor> acceptor_;
    std::function<std::unique_ptr<IMessageCodec>()> codecFactory_;
    std::unordered_map<ClientId, std::unique_ptr<Session>> sessions_;
    ServerDispatcher dispatcher_;
    std::vector<ClientId> pendingClose_;
    ClientId nextId{0};
};
