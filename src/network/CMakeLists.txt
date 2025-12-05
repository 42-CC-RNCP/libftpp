# src/network/CMakeLists.txt

add_library(network STATIC
    message.cpp
)

target_include_directories(network
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)
