# src/threading/CMakeLists.txt

add_library(threading STATIC
    thread.cpp
    # worker_pool.cpp
)

target_include_directories(threading
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(threading
    INTERFACE
        tpp_iostream
)
