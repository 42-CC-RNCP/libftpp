# src/iostream/CMakeLists.txt

add_library(tpp_iostream STATIC
    thread_safe_iostream.cpp
)

target_include_directories(tpp_iostream
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)
