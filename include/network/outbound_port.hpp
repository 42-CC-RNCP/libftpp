#pragma once
#include "connection.hpp"


class OutboundPort {
public:
    explicit OutboundPort(Connection& c);
    void send(Message&& msg);      // direct: conn.queue(...)
    void disconnect();

private:
    Connection* conn_;
};
