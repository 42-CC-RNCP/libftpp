# src/data_structures/CMakeLists.txt

add_library(data_structures STATIC
    data_buffer.cpp
)

target_include_directories(data_structures
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)
