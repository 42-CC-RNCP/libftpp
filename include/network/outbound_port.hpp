#pragma once
#include "connection.hpp"

class OutboundPort
{
public:
    explicit OutboundPort(Connection& c) : conn_(&c) {}
    void send(Message&& msg) { conn_->queue(std::move(msg)); }
    void disconnect() { conn_->close(); }

private:
    Connection* conn_;
};
