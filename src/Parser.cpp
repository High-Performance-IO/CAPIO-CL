#include <filesystem>
#include <fstream>

#include "capiocl.hpp"
#include "capiocl/engine.h"
#include "capiocl/parser.h"
#include "capiocl/printer.h"

capiocl::parser::ParserException::ParserException(const std::string &msg) : message(msg) {
    printer::print(printer::CLI_LEVEL_ERROR, msg);
}

jsoncons::jsonschema::json_schema<jsoncons::json>
capiocl::parser::Parser::loadSchema(const char *data) {
    return jsoncons::jsonschema::make_json_schema(jsoncons::json::parse(data));
}

std::filesystem::path capiocl::parser::Parser::resolve(std::filesystem::path path,
                                                       const std::filesystem::path &prefix) {
    if (prefix.empty()) {
        return path;
    }

    if (path.is_absolute()) {
        return path;
    }

    auto resolved  = prefix / path;
    const auto msg = "Path : " + path.string() + " IS RELATIVE! Resolved to: " + resolved.string();
    printer::print(printer::CLI_LEVEL_WARNING, msg);

    return resolved;
}

void capiocl::parser::Parser::validate_json(const jsoncons::json &doc, const char *str_schema) {
    jsoncons::jsonschema::json_schema<jsoncons::json> schema = loadSchema(str_schema);
    try {
        // throws jsoncons::jsonschema::validation_error on failure
        [[maybe_unused]] auto status = schema.validate(doc);
    } catch (const jsoncons::jsonschema::validation_error &e) {
        printer::print(printer::CLI_LEVEL_ERROR, e.what());
        throw ParserException("JSON validation failed!");
    }
}

capiocl::engine::Engine *capiocl::parser::Parser::parse(const std::filesystem::path &source,
                                                        const std::filesystem::path &resolve_prefix,
                                                        bool store_only_in_memory) {

    if (source.empty()) {
        throw ParserException("Empty source file name!");
    }

    std::ifstream file(source);
    if (!file.is_open()) {
        throw ParserException("Failed to open file!");
    }
    std::string capio_cl_release;
    {
        jsoncons::json doc = jsoncons::json::parse(file);
        if (!doc.contains("version")) {
            capio_cl_release = CAPIO_CL_VERSION::V1;
        } else {
            capio_cl_release = doc["version"].as<std::string>();
        }
    }

    file.close();
    printer::print(printer::CLI_LEVEL_INFO,
                   "Parsing CAPIO-CL config file for version: " + capio_cl_release);

    if (capio_cl_release == CAPIO_CL_VERSION::V1) {
        return available_parsers::parse_v1(source, resolve_prefix, store_only_in_memory);
    } else if (capio_cl_release == CAPIO_CL_VERSION::V1_1) {
        return available_parsers::parse_v1_1(source, resolve_prefix, store_only_in_memory);
    } else {
        throw ParserException("Invalid CAPIO-CL specification version!");
    }
}