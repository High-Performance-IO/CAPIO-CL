#include "capio_cl_json_schemas.hpp"
#include "capiocl.hpp"
#include "capiocl/engine.h"
#include "capiocl/parser.h"
#include "capiocl/printer.h"

capiocl::engine::Engine *
capiocl::parser::Parser::available_parsers::parse_v2(const std::filesystem::path &source,
                                                     const std::filesystem::path &resolve_prefix,
                                                     bool store_only_in_memory) {
    return {};
}