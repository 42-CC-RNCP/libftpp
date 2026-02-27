# src/design_patterns/CMakeLists.txt

add_library(design_patterns INTERFACE)

target_include_directories(design_patterns
    PUBLIC
        ${LIBFTPP_INCLUDE_DIR}
)

target_link_libraries(design_patterns
    PUBLIC
        data_structures
)