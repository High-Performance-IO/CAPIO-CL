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
#include "capiocl/serializer.h"

capiocl::engine::Engine *parseConfiguration(
    const std::filesystem::path &source, const std::filesystem::path &resolve_prefix = "",
    bool store_only_in_memory = false) {
    return capiocl::parser::Parser::parse(capiocl::configuration::CapioClConfiguration({
        {"capiocl.config_path", source.string()},
        {"capiocl.resolve_path", resolve_prefix.string()},
        {"capiocl.store_all_in_memory", store_only_in_memory ? "true" : "false"},
    }));
}

#include "test_apis.hpp"
#include "test_configuration.hpp"
#include "test_engine.hpp"
#include "test_exceptions.hpp"
#include "test_monitor.hpp"
#include "test_runtime_commit.hpp"
#include "test_serialize_deserialize.hpp"
