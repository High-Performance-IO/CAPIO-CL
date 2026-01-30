#include "capiocl.hpp"
#include <cstdlib>
#include <cstring>
#include <cxxabi.h>
#include <gtest/gtest.h>

const std::vector<std::string> CAPIO_CL_AVAIL_VERSIONS = {capiocl::CAPIO_CL_VERSION::V1,
                                                          capiocl::CAPIO_CL_VERSION::V1_1};

template <typename T> std::string demangled_name(const T &obj) {
    int status;
    const char *mangled = typeid(obj).name();
    std::unique_ptr<char, void (*)(void *)> demangled(
        abi::__cxa_demangle(mangled, nullptr, nullptr, &status), std::free);
    return status == 0 ? demangled.get() : mangled;
}

#include "capiocl/engine.h"
#include "capiocl/monitor.h"
#include "capiocl/parser.h"
#include "capiocl/printer.h"
#include "capiocl/serializer.h"

#include "test_apis.hpp"
#include "test_configuration.hpp"
#include "test_engine.hpp"
#include "test_exceptions.hpp"
#include "test_monitor.hpp"
#include "test_serialize_deserialize.hpp"