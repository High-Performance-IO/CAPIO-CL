#include <filesystem>
#include <fstream>

#include "calf/StdOutLogger.h"
#include "calf/StlLogger.h"
#include "capiocl.hpp"
#include "capiocl/engine.h"
#include "capiocl/parser.h"

capiocl::parser::ParserException::ParserException(const std::string &msg) : message(msg) {
    START_LOG(calf_current_tid(), "call()");
    UPDATE_CALF_WORKFLOW_NAME("");
    CALF_PRINT_COLOR(CALF_CLI_LEVEL_ERROR, "%s", msg.c_str());
}

jsoncons::jsonschema::json_schema<jsoncons::json>
capiocl::parser::Parser::loadSchema(const char *data) {
    START_LOG(calf_current_tid(), "call()");
    return jsoncons::jsonschema::make_json_schema(jsoncons::json::parse(data));
}

std::filesystem::path capiocl::parser::Parser::resolve(std::filesystem::path path,
                                                       const std::filesystem::path &prefix) {
    START_LOG(calf_current_tid(), "call()");
    UPDATE_CALF_WORKFLOW_NAME("");
    if (prefix.empty()) {
        return path;
    }

    if (path.is_absolute()) {
        return path;
    }

    auto resolved = prefix / path;
    CALF_PRINT_COLOR(CALF_CLI_LEVEL_WARNING, "Path %s IS RELATIVE! Resolved to: %s", path.c_str(),
                     resolved.c_str());

    return resolved;
}

void capiocl::parser::Parser::validate_json(const jsoncons::json &doc, const char *str_schema) {
    START_LOG(calf_current_tid(), "call()");
    jsoncons::jsonschema::json_schema<jsoncons::json> schema = loadSchema(str_schema);
    try {
        // throws jsoncons::jsonschema::validation_error on failure
        schema.validate(doc);
    } catch (const jsoncons::jsonschema::validation_error &e) {
        LOG("JSON validation failed error=%s", e.what());
        UPDATE_CALF_WORKFLOW_NAME("");
        CALF_PRINT_COLOR(CALF_CLI_LEVEL_ERROR, "%s", e.what());
        throw ParserException("JSON validation failed!");
    }
}

capiocl::engine::Engine *
capiocl::parser::Parser::parse(const configuration::CapioClConfiguration &config) {

    START_LOG(calf_current_tid(), "call()");

    std::string source, resolve_prefix, store_only_in_memory, dynamic_api_enabled;
    config.getParameter("capiocl.config_path", &source, "");
    config.getParameter("capiocl.resolve_path", &resolve_prefix, "");
    config.getParameter("capiocl.store_all_in_memory", &store_only_in_memory, "false");
    config.getParameter("capiocl.dynamic_api.enabled", &dynamic_api_enabled, "false");

    LOG("parse requested source=%s resolve_prefix=%s memory_only=%s", source.c_str(),
        resolve_prefix.c_str(), store_only_in_memory.c_str());

    if (source.empty()) {
        auto *engine = new engine::Engine(config);
        if (store_only_in_memory == "true") {
            engine->setAllStoreInMemory();
        }
        if (dynamic_api_enabled == "true") {
            engine->startApiServer();
        }
        return engine;
    }

    std::ifstream file(source);
    if (!file.is_open()) {
        LOG("failed to open parse source=%s", source.c_str());
        throw ParserException("Failed to open file!");
    }
    std::string capio_cl_release;
    {
        jsoncons::json doc;
        try {
            doc = jsoncons::json::parse(file);
        } catch (const jsoncons::json_exception &e) {
            LOG("failed to decode JSON source=%s error=%s", source.c_str(), e.what());
            throw;
        }
        if (!doc.contains("version")) {
            capio_cl_release = CAPIO_CL_VERSION::V1;
        } else {
            capio_cl_release = doc["version"].as<std::string>();
        }
    }

    file.close();

    UPDATE_CALF_WORKFLOW_NAME("");
    CALF_PRINT_COLOR(CALF_CLI_LEVEL_INFO, "Parsing CAPIO-CL config file for version:  %s",
                     capio_cl_release.c_str());

    engine::Engine *engine;
    if (capio_cl_release == CAPIO_CL_VERSION::V1) {
        engine = available_parsers::parse_v1(source, resolve_prefix, store_only_in_memory == "true",
                                             &config);
    } else if (capio_cl_release == CAPIO_CL_VERSION::V1_1) {
        engine = available_parsers::parse_v1_1(source, resolve_prefix,
                                               store_only_in_memory == "true", &config);
    } else {
        LOG("unsupported specification version=%s source=%s", capio_cl_release.c_str(),
            source.c_str());
        throw ParserException("Invalid CAPIO-CL specification version!");
    }

    if (dynamic_api_enabled == "true") {
        engine->startApiServer();
    }
    return engine;
}
