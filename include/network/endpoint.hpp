#pragma once
#include <cstdint>
#include <string>

// a simple wrapper
struct Endpoint
{
    std::string ip;     // "127.0.0.1"
    std::uint16_t port; // 8080
};
