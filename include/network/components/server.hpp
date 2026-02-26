// network/components/server.hpp
#pragma once
#include "connection.hpp"
#include "dispatcher.hpp"
#include "network/contracts/acceptor.hpp"
#include "network/contracts/network_port.hpp"
#include "network/contracts/reactor.hpp"
#include "network/core/message.hpp"
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
           std::unique_ptr<IReactor> reactor,
           std::function<std::unique_ptr<IMessageCodec>()> codecFactory) :
        acceptor_(std::move(acceptor)),
        reactor_(std::move(reactor)),
        codecFactory_(std::move(codecFactory))
    {
    }

    void start(const size_t& p_port)
    {
        acceptor_->listen(static_cast<std::uint16_t>(p_port));

        // register listening socket to reactor
        reactor_->add(
            acceptor_->nativeHandle(), IoEvent::Readable, [this](int, IoEvent) {
                _onAccept();
            });
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
            _updateSessionEvents(*it->second);
        }
    }

    void sendToAll(const Message& message) override
    {
        for (auto& [id, session] : sessions_) {
            session->conn.queue(message);
            _updateSessionEvents(*session);
        }
    }

    void sendToArray(const Message& message, std::vector<ClientId> clientIDs)
    {
        for (auto& id : clientIDs) {
            sendTo(message, id);
        }
    }

    void disconnect(ClientId clientID) override { sessions_.erase(clientID); }

    // timeout_ms: maximum time to wait for events, 0 means non-blocking, -1
    // means block indefinitely
    void update(int timeout_ms = 0)
    {
        reactor_->poll(timeout_ms);
        _processPendingClose();
    }

private:
    void _onAccept()
    {
        while (auto t = acceptor_->tryAccept()) {
            ClientId id = nextId_++;
            int fd = t->nativeHandle();

            auto codec = codecFactory_();
            auto session = std::make_unique<Session>(
                id, this, std::move(t), std::move(codec));
            sessions_.emplace(id, std::move(session));

            // register event for the client session to reactor
            reactor_->add(fd, IoEvent::Readable, [this, id](int, IoEvent ev) {
                _onSessionEvent(id, ev);
            });
        }
    }

    void _onSessionEvent(ClientId id, IoEvent events)
    {
        auto it = sessions_.find(id);
        if (it == sessions_.end()) {
            return;
        }

        Session& s = *it->second;

        if (hasEvent(events, IoEvent::Error)
            || hasEvent(events, IoEvent::Closed)) {
            _closeLater(id);
            return;
        }

        if (hasEvent(events, IoEvent::Readable)) {
            _processSessionRead(s);
        }

        if (hasEvent(events, IoEvent::Writable)) {
            _processSessionWrite(s);
        }
    }

    void _processSessionRead(Session& s)
    {
        // process incoming messages
        auto io = s.conn.onReadable();

        if (io.status == Connection::IoStatus::Closed
            || io.status == Connection::IoStatus::Error) {
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
    }

    void _processSessionWrite(Session& s)
    {
        // process outgoing messages
        if (s.conn.wantsWrite()) {
            auto wo = s.conn.onWritable();

            if (wo.status == Connection::IoStatus::Closed
                || wo.status == Connection::IoStatus::Error) {
                _closeLater(s.id);
                return;
            }
            _updateSessionEvents(s);
        }
    }

    void _processPendingClose()
    {
        for (ClientId id : pendingClose_) {
            auto it = sessions_.find(id);
            if (it != sessions_.end()) {
                int fd = it->second->transport->nativeHandle();

                // remove the client session from reactor
                reactor_->remove(fd);
                // close the connection
                it->second->conn.close();
                sessions_.erase(it);
            }
        }
        pendingClose_.clear();
    }

    void _updateSessionEvents(Session& s)
    {
        IoEvent events = IoEvent::Readable;
        if (s.conn.wantsWrite()) {
            events = static_cast<IoEvent>(
                static_cast<int>(events) | static_cast<int>(IoEvent::Writable));
        }
        reactor_->modify(s.transport->nativeHandle(), events);
    }

    void _closeLater(ClientId id) { pendingClose_.push_back(id); }

private:
    std::unique_ptr<IAcceptor> acceptor_;
    std::unique_ptr<IReactor> reactor_;
    std::function<std::unique_ptr<IMessageCodec>()> codecFactory_;
    std::unordered_map<ClientId, std::unique_ptr<Session>> sessions_;
    ServerDispatcher dispatcher_;
    std::vector<ClientId> pendingClose_;
    ClientId nextId_{0};
};
