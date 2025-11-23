cmake_minimum_required(VERSION 3.16)
project(libftpp)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_options(-Wall -Wextra -Werror)

# ---- GoogleTest ----
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
FetchContent_MakeAvailable(googletest)

enable_testing()

# ---- Module libraries ----
add_subdirectory(src/data_structures)
add_subdirectory(src/design_patterns)
add_subdirectory(src/iostream)
add_subdirectory(src/mathematics)
add_subdirectory(src/network)
add_subdirectory(src/threading)

add_library(libftpp INTERFACE)

target_include_directories(libftpp
    INTERFACE
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(libftpp
    INTERFACE
        data_structures
        design_patterns
        tpp_iostream
        mathematics
        network
        threading
)

# ---- Tests ----
add_subdirectory(tests)
