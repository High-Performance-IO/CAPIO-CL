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
    message(STATUS "CAPIO-CL included as subproject → tests disabled")
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
        pybind11
        GIT_REPOSITORY https://github.com/pybind/pybind11.git
        GIT_TAG v3.0.1
)

FetchContent_MakeAvailable(nlohmann_json pybind11)

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
)


#####################################
# Install rules
#####################################
install(TARGETS ${TARGET_NAME}
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
    target_link_libraries(${PYTHON_BIND_NAME}
            PRIVATE libcapio_cl
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
    # Install rules
    #####################################
    # Optionally install tests
    install(TARGETS CAPIO_CL_tests
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif ()
