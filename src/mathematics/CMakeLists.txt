# src/mathematics/CMakeLists.txt

add_library(mathematics STATIC
    mathematics.cpp
)

target_include_directories(mathematics
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)
