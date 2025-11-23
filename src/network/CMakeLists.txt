# src/network/CMakeLists.txt

add_library(network INTERFACE)

target_include_directories(network
    INTERFACE
        ${PROJECT_SOURCE_DIR}/include
)
