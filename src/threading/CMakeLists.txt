# src/threading/CMakeLists.txt

add_library(threading INTERFACE)

target_include_directories(threading
    INTERFACE
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(threading
    INTERFACE
        tpp_iostream
)
