#####################################
# Project-wide settings
#####################################
cmake_minimum_required(VERSION 3.15)

project(CAPIO-CL
        LANGUAGES CXX
        DESCRIPTION "Cross-Application Programmable IO - Coordination Language"
        VERSION 1.0.0
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -pedantic -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)
include(GNUInstallDirs)

#####################################
# Options
#####################################
option(CAPIO_CL_BUILD_TESTS "Build CAPIO-CL test suite" ON)
option(BUILD_PYTHON_BINDINGS "Build python bindings for CAPIO-CL" ON)
option(ENABLE_COVERAGE "Enable code coverage collection" FALSE)

# Disable tests when included as a subproject
if (NOT PROJECT_IS_TOP_LEVEL)
    set(CAPIO_CL_BUILD_TESTS OFF)
    set(BUILD_PYTHON_BINDINGS OFF)
    message(STATUS "CAPIO-CL included as subproject → tests disabled")
    message(STATUS "CAPIO-CL included as subproject → python bindings disabled")
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-O0 -g)
endif ()

if (ENABLE_COVERAGE)
    message(STATUS "Building with code coverage instrumentation")
    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)
endif ()

#####################################
# External projects
#####################################
# simdjson
FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.12.0
)

FetchContent_Declare(
        valijson
        GIT_REPOSITORY https://github.com/tristanpenman/valijson.git
        GIT_TAG master   # Use master as it is the branch that targets C++17
)
set(valijson_BUILD_TESTS OFF CACHE BOOL "Disable building tests for Valijson" FORCE)
set(valijson_BUILD_EXAMPLES OFF CACHE BOOL "Disable building examples for Valijson" FORCE)


FetchContent_MakeAvailable(nlohmann_json valijson)


if (BUILD_PYTHON_BINDINGS)

    # Add pybind11 project only ehn building python bindings

    FetchContent_Declare(
            pybind11
            GIT_REPOSITORY https://github.com/pybind/pybind11.git
            GIT_TAG v3.0.1
    )
    FetchContent_MakeAvailable(pybind11)
endif ()

#####################################
# Encode CAPIO-CL JSON Schemas
#####################################
set(CAPIOCL_JSON_SCHEMAS_DIRECTORY "${CMAKE_BINARY_DIR}/schemas")
set(OUTPUT_HEADER "${CAPIOCL_JSON_SCHEMAS_DIRECTORY}/capio_cl_json_schemas.hpp")
file(GLOB SCHEMA_FILES "${CMAKE_SOURCE_DIR}/schema/*.json")

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/schema*.json")

file(WRITE ${OUTPUT_HEADER} "// Bundled CAPIO-CL encoded JSON schemas\n")
file(APPEND ${OUTPUT_HEADER} "#pragma once\n\n")


foreach (SCHEMA_FILE ${SCHEMA_FILES})

    get_filename_component(SCHEMA_NAME ${SCHEMA_FILE} NAME_WE)
    get_filename_component(SCHEMA_BASENAME ${SCHEMA_FILE} NAME)
    message(STATUS "Bundling CAPIO-CL schema: ${SCHEMA_BASENAME}")
    file(APPEND ${OUTPUT_HEADER} "// CAPIO-CL version: ${SCHEMA_NAME}\n")

    # Generate an encoded version of jsons schemas to be bundled win the binary
    # Run with the wording directory = the source schema folder to have variables
    # that do not encode the full path in the schema variable name i.e.
    # v1_json instead of _home_user_CAPIO-CL_schema_v1_json

    execute_process(
            COMMAND xxd -i ${SCHEMA_BASENAME}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/schema
            OUTPUT_VARIABLE HEXDATA
            RESULT_VARIABLE RES
    )
    if (NOT RES EQUAL 0)
        message(FATAL_ERROR "xxd failed for ${SCHEMA_FILE}")
    endif ()

    file(APPEND ${OUTPUT_HEADER} "${HEXDATA}\n\n")
endforeach ()


message(STATUS "Generated header: ${OUTPUT_HEADER}")


#####################################
# Sources and headers
#####################################
file(GLOB_RECURSE CAPIO_SRC CONFIGURE_DEPENDS "src/*.cpp")
set(CAPIO_CL_HEADERS capiocl.hpp)

# Library target (for use with external projects)
add_library(libcapio_cl STATIC ${CAPIO_SRC} ${CAPIO_CL_HEADERS})

target_include_directories(libcapio_cl PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${nlohmann_json_SOURCE_DIR}/include
        ${CAPIOCL_JSON_SCHEMAS_DIRECTORY}
)

target_link_libraries(libcapio_cl
        PUBLIC
        nlohmann_json::nlohmann_json
        ValiJSON::valijson
)

#####################################
# Install rules
#####################################
install(TARGETS libcapio_cl
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)


#####################################
# Python bindings
#####################################
if (BUILD_PYTHON_BINDINGS)

    set(PYTHON_BIND_NAME _py_capio_cl)

    pybind11_add_module(${PYTHON_BIND_NAME}
            bindings/python_bindings.cpp
    )


    # Make sure the binding sees the CAPIO CL headers & links against the core lib
    target_link_libraries(${PYTHON_BIND_NAME} PRIVATE
            libcapio_cl
            ValiJSON::valijson
    )

    target_include_directories(${PYTHON_BIND_NAME}
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
    )

    install(TARGETS _py_capio_cl DESTINATION py_capio_cl)

endif ()


#####################################
# Tests (only when built standalone)
#####################################
if (CAPIO_CL_BUILD_TESTS)
    message(STATUS "Building CAPIO-CL tests")

    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.14.0
    )

    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_executable(CAPIO_CL_tests tests/cpp/main.cpp)

    target_link_libraries(CAPIO_CL_tests PRIVATE
            libcapio_cl
            ValiJSON::valijson
            GTest::gtest_main
    )

    target_include_directories(CAPIO_CL_tests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    include(GoogleTest)
    gtest_discover_tests(CAPIO_CL_tests)

    #####################################
    # Copy JSON test files
    #####################################
    set(TEST_JSON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests/jsons")

    add_custom_command(
            TARGET CAPIO_CL_tests PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "/tmp/capio_cl_jsons"
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${TEST_JSON_DIR} "/tmp/capio_cl_jsons"
            COMMENT "Copying JSON test files to build directory"
    )

    #####################################
    # Install rules
    #####################################
    # Optionally install tests
    install(TARGETS CAPIO_CL_tests
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    # Copy tests jsons
    install(DIRECTORY ${TEST_JSON_DIR}
            DESTINATION ${CMAKE_INSTALL_BINDIR}/jsons
            FILES_MATCHING PATTERN "*.json"
    )
endif ()
