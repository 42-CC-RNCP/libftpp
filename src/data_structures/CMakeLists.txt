# src/data_structures/CMakeLists.txt
set(MODULE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(CPP2_GEN_DIR ${CMAKE_BINARY_DIR}/cpp2gen/data_structures)
file(MAKE_DIRECTORY ${CPP2_GEN_DIR})

file(GLOB CPP_FILES "${MODULE_SOURCE_DIR}/*.cpp")
file(GLOB CPP2_FILES "${MODULE_SOURCE_DIR}/*.cpp2")

set(GENERATED_CPP_FILES "")
foreach(cpp2_file ${CPP2_FILES})
    get_filename_component(filename_we ${cpp2_file} NAME_WE)
    set(generated_cpp "${CPP2_GEN_DIR}/${filename_we}.cpp")

    add_custom_command(
        OUTPUT ${generated_cpp}
        COMMAND cppfront ${cpp2_file} -o ${generated_cpp}
        DEPENDS ${cpp2_file}
        COMMENT "Translating ${cpp2_file} to ${generated_cpp}"
    )
    list(APPEND GENERATED_CPP_FILES ${generated_cpp})
endforeach()

add_custom_target(data_structures_cpp2_gen DEPENDS ${GENERATED_CPP_FILES})

if (CPP_FILES OR GENERATED_CPP_FILES)
    add_library(data_structures STATIC
        ${CPP_FILES}
        ${GENERATED_CPP_FILES}
    )
    add_dependencies(data_structures data_structures_cpp2_gen)

    target_include_directories(data_structures PUBLIC
        ${PROJECT_SOURCE_DIR}/include/data_structures
    )
else()
    add_library(data_structures INTERFACE)

    target_include_directories(data_structures INTERFACE
        ${PROJECT_SOURCE_DIR}/include/data_structures
    )
endif()
