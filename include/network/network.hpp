// include/network/network.hpp
#pragma once

// Core
#include "core/endpoint.hpp"
#include "core/message.hpp"

// Public API
#include "components/client.hpp"
#include "components/message_builder.hpp"
#include "components/outbound_port.hpp"
#include "components/peer_handle.hpp"
#include "components/server.hpp"

// Implementations
#include "impl/codec/length_prefixed_codec.hpp"
#include "impl/reactor/epoll_reactor.hpp"
#include "impl/tcp/tcp_acceptor.hpp"
#include "impl/tcp/tcp_transport.hpp"
