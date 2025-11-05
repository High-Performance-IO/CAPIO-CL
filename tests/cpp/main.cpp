#include "capiocl.hpp"

#include <cstdlib>
#include <cstring>
#include <cxxabi.h>
#include <gtest/gtest.h>

template <typename T> std::string demangled_name(const T &obj) {
    int status;
    const char *mangled = typeid(obj).name();
    std::unique_ptr<char, void (*)(void *)> demangled(
        abi::__cxa_demangle(mangled, nullptr, nullptr, &status), std::free);
    return status == 0 ? demangled.get() : mangled;
}

#include "test_engine.hpp"
#include "test_exceptions.hpp"
#include "test_monitor.hpp"
#include "test_serialize_deserialize.hpp"