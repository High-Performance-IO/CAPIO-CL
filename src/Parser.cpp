#include "capio_cl_json_schemas.hpp"
#include "capiocl.hpp"

#include <filesystem>
#include <fstream>
#include <jsoncons/json.hpp>
#include <jsoncons_ext/jsonschema/jsonschema.hpp>

std::filesystem::path resolve(std::filesystem::path path, const std::filesystem::path &prefix) {
    if (prefix.empty()) {
        return path;
    }

    if (path.is_absolute()) {
        return path;
    }

    auto resolved  = prefix / path;
    const auto msg = "Path : " + path.string() + " IS RELATIVE! Resolved to: " + resolved.string();
    capiocl::print_message(capiocl::CLI_LEVEL_WARNING, msg);

    return resolved;
}

jsoncons::jsonschema::json_schema<jsoncons::json> loadSchema(const unsigned char *data,
                                                             unsigned int len) {
    std::string schemaStr(reinterpret_cast<const char *>(data), len);
    jsoncons::json schemaJson = jsoncons::json::parse(schemaStr);

    return jsoncons::jsonschema::make_json_schema(schemaJson);
}

void validate_json(const jsoncons::json &doc) {
    jsoncons::jsonschema::json_schema<jsoncons::json> schema = loadSchema(v1_json, v1_json_len);
    try {
        schema.validate(doc); // throws jsoncons::jsonschema::validation_error on failure
    } catch (const jsoncons::jsonschema::validation_error &e) {
        capiocl::print_message(capiocl::CLI_LEVEL_ERROR, e.what());
        throw capiocl::ParserException("JSON validation failed!");
    }
}

#include "parsers/v1.hpp"

std::tuple<std::string, capiocl::Engine *>
capiocl::Parser::parse(const std::filesystem::path &source,
                       const std::filesystem::path &resolve_prefix, bool store_only_in_memory) {

    if (source.empty()) {
        throw capiocl::ParserException("Empty source file name!");
    }

    std::ifstream file(source);
    if (!file.is_open()) {
        throw capiocl::ParserException("Failed to open file!");
    }
    std::string CAPIO_CL_version;
    {
        jsoncons::json doc = jsoncons::json::parse(file);
        if (!doc.contains("version")) {
            CAPIO_CL_version = "1.0";
        } else {
            CAPIO_CL_version = doc["version"].as<std::string>();
        }
    }

    file.close();
    print_message(CLI_LEVEL_INFO, "Parsing CAPIO-CL config file for version: " + CAPIO_CL_version);

    if (CAPIO_CL_version == "1.0") {
        return parse_v1(source, resolve_prefix, store_only_in_memory);
    } else {
        throw ParserException("Invalid CAPIO-CL specification version!");
    }
}