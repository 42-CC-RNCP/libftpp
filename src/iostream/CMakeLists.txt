# src/iostream/CMakeLists.txt

add_library(tpp_iostream INTERFACE)

target_include_directories(tpp_iostream
    INTERFACE
        ${PROJECT_SOURCE_DIR}/include
)
