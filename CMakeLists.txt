cmake_minimum_required(VERSION 3.16)
project(libftpp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Werror)

add_subdirectory(src/data_structures)
