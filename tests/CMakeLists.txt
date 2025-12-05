# tests/CMakeLists.txt

include_directories(${PROJECT_SOURCE_DIR}/include)

include(GoogleTest)

function(add_libtpp_test name)
  set(options)
  set(oneValueArgs)
  set(multiValueArgs SRCS LIBS)
  cmake_parse_arguments(T "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  add_executable(${name} ${T_SRCS})
  target_link_libraries(${name} PRIVATE gtest_main ${T_LIBS})

  set(TEST_TARGETS ${TEST_TARGETS} ${name} PARENT_SCOPE)
endfunction()

add_libtpp_test(test_data_structures
  SRCS
    pool_test.cpp
    tlv_test.cpp
    databuffer_core_test.cpp
    databuffer_containers_test.cpp
    # databuffer_objects_test.cpp
    databuffer_special_cases_test.cpp
  LIBS
    data_structures
)

add_libtpp_test(test_memento
  SRCS
    memento_test.cpp
    memento_history_test.cpp
  LIBS
    design_patterns
    data_structures
)

add_libtpp_test(test_observer
  SRCS
    observer_test.cpp
  LIBS
    design_patterns
)

add_libtpp_test(test_singleton
  SRCS
    singleton_test.cpp
  LIBS
    design_patterns
)

add_libtpp_test(test_state_machine
  SRCS
    state_machine_test.cpp
  LIBS
    design_patterns
)

add_libtpp_test(test_threading
  SRCS
    thread_safe_iostream_test.cpp
    thread_safe_queue_test.cpp
    thread_test.cpp
    worker_pool_test.cpp
  LIBS
    threading
)

add_libtpp_test(test_network
  SRCS
    message_test.cpp
  LIBS
    data_structures
    network
)

foreach(t IN LISTS TEST_TARGETS)
  gtest_discover_tests(${t}
    TEST_PREFIX "${t}."
    DISCOVERY_TIMEOUT 60
  )
  set_property(TARGET ${t} PROPERTY EXCLUDE_FROM_ALL TRUE)
endforeach()

add_custom_target(libftpp_tests
  DEPENDS ${TEST_TARGETS}
)

set_property(TARGET libftpp_tests PROPERTY EXCLUDE_FROM_ALL TRUE)
