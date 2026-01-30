#####################################
# Project-wide settings
#####################################
cmake_minimum_required(VERSION 3.15)

project(CAPIO-CL
        LANGUAGES CXX
        DESCRIPTION "Cross-Application Programmable IO - Coordination Language"
        VERSION 1.2.0
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
option(CAPIO_CL_BUILD_TESTS "Build CAPIO-CL test suite" OFF)
option(BUILD_PYTHON_BINDINGS "Build python bindings for CAPIO-CL" OFF)
option(ENABLE_COVERAGE "Enable code coverage collection" FALSE)
option(ENABLE_COVERAGE_PIPELINE "Add dedicated target to execute and collect coverage" OFF)

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
        tomlplusplus
        GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
        GIT_TAG v3.4.0
)

FetchContent_Declare(
        jsoncons
        GIT_REPOSITORY https://github.com/danielaparker/jsoncons.git
        GIT_TAG v1.4.3
)

FetchContent_Declare(
        httplib
        GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
        GIT_TAG v0.29.0
)


set(JSONCONS_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(JSONCONS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(JSONCONS_BUILD_FUZZERS OFF CACHE BOOL "" FORCE)
set(HTTPLIB_USE_ZSTD_IF_AVAILABLE OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(jsoncons tomlplusplus httplib)

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

set(CAPIOCL_JSON_SCHEMAS_DIRECTORY "${CMAKE_BINARY_DIR}/schemas")
set(OUTPUT_HEADER "${CAPIOCL_JSON_SCHEMAS_DIRECTORY}/capio_cl_json_schemas.hpp")
file(GLOB SCHEMA_FILES "${CMAKE_CURRENT_SOURCE_DIR}/schema/*.json")

message(STATUS "Generating output header at ${OUTPUT_HEADER}")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/schema/*.json")

file(MAKE_DIRECTORY ${CAPIOCL_JSON_SCHEMAS_DIRECTORY})
file(WRITE ${OUTPUT_HEADER} "// Bundled CAPIO-CL encoded JSON schemas\n")
file(APPEND ${OUTPUT_HEADER} "#pragma once\n\n")

foreach (SCHEMA_FILE ${SCHEMA_FILES})
    get_filename_component(SCHEMA_BASENAME ${SCHEMA_FILE} NAME)
    message(STATUS "Bundling CAPIO-CL schema: ${SCHEMA_BASENAME}")

    execute_process(
            COMMAND bash -c "echo ${SCHEMA_BASENAME} | sed 's/\\.json$//'"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/schema
            OUTPUT_VARIABLE SCHEMA_NAME_RAW
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    string(REPLACE "." "_" SCHEMA_NAME "${SCHEMA_NAME_RAW}")

    execute_process(
            COMMAND bash -c
            "echo \"// CAPIO-CL version: ${SCHEMA_NAME_RAW}\" && \
             echo \"constexpr char schema_${SCHEMA_NAME}[] = R\\\"(\" && \
             cat \"${SCHEMA_BASENAME}\" && \
             echo \")\\\";\""
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/schema
            OUTPUT_VARIABLE HEXDATA
            RESULT_VARIABLE RES
    )

    if (NOT RES EQUAL 0)
        message(FATAL_ERROR "Failed to bundle ${SCHEMA_BASENAME}: exit ${RES}")
    endif ()

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
        ${TOMLPLUSPLUS_SOURCE_DIR}/include
        ${httplib_SOURCE_DIR}
)

target_link_libraries(libcapio_cl PUBLIC)
target_link_libraries(libcapio_cl PRIVATE
        tomlplusplus::tomlplusplus
        httplib::httplib
)

find_library(LIBANL anl)
if(LIBANL)
    target_link_libraries(libcapio_cl PRIVATE ${LIBANL})
endif ()

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

    find_package(CURL REQUIRED)

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
            CURL::libcurl
    )

    if(LIBANL)
        target_link_libraries(CAPIO_CL_tests PRIVATE ${LIBANL})
    endif ()

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
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${TEST_JSON_DIR}"
            "/tmp/capio_cl_jsons"
            COMMENT "Copying JSON test files with full directory structure"
    )

    #####################################
    # Copy TOMLS test files
    #####################################
    set(TEST_TOMLS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests/tomls")

    add_custom_command(
            TARGET CAPIO_CL_tests PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "/tmp/capio_cl_tomls"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${TEST_TOMLS_DIR}"
            "/tmp/capio_cl_tomls"
            COMMENT "Copying TOMLS test files with full directory structure"
    )

    #####################################
    # Install rules for tests
    #####################################
    install(TARGETS CAPIO_CL_tests
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    #####################################
    # Target to execute tests
    #####################################
    add_custom_target(run_tests
            DEPENDS CAPIO_CL_tests
            COMMAND $<TARGET_FILE:CAPIO_CL_tests>
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Executing CAPIO_CL_tests"
    )

endif ()

if (ENABLE_COVERAGE_PIPELINE)

    if (NOT (CAPIO_CL_BUILD_TESTS AND ENABLE_COVERAGE))
        message(FATAL_ERROR "Unable to add coverage pipeline when coverage and tests are not being built")
    endif ()

    find_program(LCOV_BIN lcov)
    find_program(GENHTML_BIN genhtml)

    if (NOT LCOV_BIN)
        message(FATAL_ERROR "lcov not found! Install lcov first.")
    endif ()

    if (NOT GENHTML_BIN)
        message(FATAL_ERROR "genhtml not found! Install lcov first.")
    endif ()

    message(STATUS "Adding coverage collection pipeline")

    # Output directories
    set(COVERAGE_DIR "${CMAKE_BINARY_DIR}/coverage")
    set(COVERAGE_INFO "${COVERAGE_DIR}/coverage.info")
    file(MAKE_DIRECTORY ${COVERAGE_DIR})

    # Run tests + collect coverage + generate HTML
    add_custom_target(generate_coverage
            DEPENDS run_tests
            COMMAND ${LCOV_BIN} --directory ${CMAKE_BINARY_DIR} --capture
            --ignore-errors inconsistent
            --ignore-errors negative
            ${GCOV_EXTRA_ARGS}
            --output-file ${COVERAGE_INFO}

            COMMAND ${LCOV_BIN}
            --remove ${COVERAGE_INFO}
                    "*jsoncons*"
                    "/usr/include/*"
                    "*googletest*"
                    "*tests/*"
                    ${COVERAGE_REMOVE_PATTERNS}
            --ignore-errors unused
            --ignore-errors inconsistent
            --output-file ${COVERAGE_INFO}

            COMMAND ${GENHTML_BIN} ${COVERAGE_INFO}
            --ignore-errors inconsistent
            --output-directory ${COVERAGE_DIR}

            COMMENT "Generating coverage report in ${COVERAGE_DIR}"
    )
endif ()
