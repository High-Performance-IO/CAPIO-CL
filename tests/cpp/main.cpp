#include <cstdlib>
#include <cstring>
#include <cxxabi.h>
#include <gtest/gtest.h>
#include "capiocl.hpp"

const std::vector<std::string> CAPIO_CL_AVAIL_VERSIONS = {capiocl::CAPIO_CL_VERSION::V1,
                                                          capiocl::CAPIO_CL_VERSION::V1_1};

template <typename T> std::string demangled_name(const T &obj) {
    int status;
    const char *mangled = typeid(obj).name();
    std::unique_ptr<char, void (*)(void *)> demangled(
        abi::__cxa_demangle(mangled, nullptr, nullptr, &status), std::free);
    return status == 0 ? demangled.get() : mangled;
}

#include "include/engine.h"
#include "include/monitor.h"
#include "include/parser.h"
#include "include/printer.h"
#include "include/serializer.h"

#include "test_engine.hpp"
#include "test_exceptions.hpp"
#include "test_monitor.hpp"
#include "test_serialize_deserialize.hpp"