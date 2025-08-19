file(GLOB TEST_SOURCES "*.cpp")

# add_executable(test_data_structures ${TEST_SOURCES})
add_executable(test_data_structures
  pool_test.cpp
  tlv_test.cpp
)

target_link_libraries(test_data_structures
    gtest_main
    data_structures
)

target_include_directories(test_data_structures PRIVATE
    ${PROJECT_SOURCE_DIR}/include/data_structures
)

include(GoogleTest)
gtest_discover_tests(test_data_structures)
