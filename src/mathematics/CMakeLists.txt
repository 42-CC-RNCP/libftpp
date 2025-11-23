# src/mathematics/CMakeLists.txt

add_library(mathematics INTERFACE)

target_include_directories(mathematics
    INTERFACE
        ${PROJECT_SOURCE_DIR}/include
)
