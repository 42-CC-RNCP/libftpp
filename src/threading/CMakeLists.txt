# src/threading/CMakeLists.txt

add_library(threading STATIC
    thread.cpp
    thread_safe_queue.cpp
)

target_include_directories(threading
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(threading
    INTERFACE
        tpp_iostream
)
