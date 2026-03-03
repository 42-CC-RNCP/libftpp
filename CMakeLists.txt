# root CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(libftpp)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_options(-Wall -Wextra -Werror)

option(LIBFTPP_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

if(LIBFTPP_ENABLE_UBSAN)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-fsanitize=undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=undefined -fno-omit-frame-pointer)
  else()
    message(WARNING "LIBFTPP_ENABLE_UBSAN is ON, but compiler does not support UBSan flags")
  endif()
endif()

# ---- GoogleTest ----
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
FetchContent_MakeAvailable(googletest)

enable_testing()
include(CTest)

find_program(VALGRIND_EXECUTABLE valgrind)
if(VALGRIND_EXECUTABLE)
  set(MEMORYCHECK_COMMAND "${VALGRIND_EXECUTABLE}")
  set(MEMORYCHECK_COMMAND_OPTIONS
      "--trace-children=yes --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=42")
endif()

# ---- Module libraries ----
set(LIBFTPP_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)

add_subdirectory(src/data_structures)
add_subdirectory(src/design_patterns)
add_subdirectory(src/iostream)
add_subdirectory(src/mathematics)
add_subdirectory(src/network)
add_subdirectory(src/threading)
add_subdirectory(examples)

add_library(libftpp INTERFACE)

target_include_directories(libftpp
    INTERFACE
        ${LIBFTPP_INCLUDE_DIR}
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

file(WRITE "${CMAKE_SOURCE_DIR}/.clangd"
"CompileFlags:\n  CompilationDatabase: build\n  Add: [-I${CMAKE_SOURCE_DIR}/include]\n")
