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

add_compile_options(-Wall -Wextra -Wpedantic)

#####################################
# External projects
#####################################

FetchContent_Declare(
        jsoncons
        GIT_REPOSITORY https://github.com/danielaparker/jsoncons.git
        GIT_TAG v1.4.3
)

set(JSONCONS_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(JSONCONS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(JSONCONS_BUILD_FUZZERS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(jsoncons)

if (BUILD_PYTHON_BINDINGS)
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
find_program(XXD_EXECUTABLE xxd)
find_program(PYTHON_EXECUTABLE python3)

set(CAPIOCL_JSON_SCHEMAS_DIRECTORY "${CMAKE_BINARY_DIR}/schemas")
set(OUTPUT_HEADER "${CAPIOCL_JSON_SCHEMAS_DIRECTORY}/capio_cl_json_schemas.hpp")
file(GLOB SCHEMA_FILES "${CMAKE_CURRENT_SOURCE_DIR}/schema/*.json")

message(STATUS "Generating output header at ${OUTPUT_HEADER}")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/schema/*.json")

file(MAKE_DIRECTORY ${CAPIOCL_JSON_SCHEMAS_DIRECTORY})
file(WRITE ${OUTPUT_HEADER} "// Bundled CAPIO-CL encoded JSON schemas\n")
file(APPEND ${OUTPUT_HEADER} "#pragma once\n\n")

foreach (SCHEMA_FILE ${SCHEMA_FILES})
    get_filename_component(SCHEMA_NAME ${SCHEMA_FILE} NAME_WE)
    get_filename_component(SCHEMA_BASENAME ${SCHEMA_FILE} NAME)
    message(STATUS "Bundling CAPIO-CL schema: ${SCHEMA_BASENAME}")

    if (XXD_EXECUTABLE)
        message(STATUS "Using system xxd: ${XXD_EXECUTABLE}")
        execute_process(
                COMMAND ${XXD_EXECUTABLE} -i ${SCHEMA_BASENAME}
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/schema
                OUTPUT_VARIABLE HEXDATA
                RESULT_VARIABLE RES
        )
        if (NOT RES EQUAL 0)
            message(FATAL_ERROR "xxd failed for ${SCHEMA_FILE}")
        endif ()
    else ()
        if (NOT PYTHON_EXECUTABLE)
            message(FATAL_ERROR "Unable to locate python3 package. either xxd or python3 packages are required to build CAPIO-CL")
        endif ()
        message(STATUS "xxd not found — using Python fallback: ${XXD_I_SCRIPT}")
        execute_process(
                COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/schema/bundler.py ${SCHEMA_BASENAME}
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/schema
                OUTPUT_VARIABLE HEXDATA
                RESULT_VARIABLE RES
        )
        if (NOT RES EQUAL 0)
            message(FATAL_ERROR "Failed bundling for ${SCHEMA_FILE}")
        endif ()
    endif ()


    file(APPEND ${OUTPUT_HEADER} "// CAPIO-CL version: ${SCHEMA_NAME}\n")
    file(APPEND ${OUTPUT_HEADER} "${HEXDATA}\n\n")
endforeach ()

message(STATUS "Generated header: ${OUTPUT_HEADER}")

#####################################
# Sources and headers
#####################################
file(GLOB_RECURSE CAPIO_SRC CONFIGURE_DEPENDS "src/*.cpp")
set(CAPIO_CL_HEADERS capiocl.hpp)

# Library target
add_library(libcapio_cl STATIC ${CAPIO_SRC} ${CAPIO_CL_HEADERS})

target_include_directories(libcapio_cl PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${jsoncons_SOURCE_DIR}/include
        ${CAPIOCL_JSON_SCHEMAS_DIRECTORY}
)

# jsoncons is header-only, no linking required
target_link_libraries(libcapio_cl PUBLIC)

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

    target_link_libraries(${PYTHON_BIND_NAME} PRIVATE
            libcapio_cl
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
    # Install rules for tests
    #####################################
    install(TARGETS CAPIO_CL_tests
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    install(DIRECTORY ${TEST_JSON_DIR}
            DESTINATION ${CMAKE_INSTALL_BINDIR}/jsons
            FILES_MATCHING PATTERN "*.json"
    )
endif ()
