#include "capiocl.hpp"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

#include <valijson/adapters/nlohmann_json_adapter.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validator.hpp>

std::filesystem::path capiocl::Parser::resolve(std::filesystem::path path,
                                               const std::filesystem::path &prefix) {
    if (prefix.empty()) {
        return path;
    }

    if (path.is_absolute()) {
        return path;
    }

    auto resolved  = prefix / path;
    const auto msg = "Path : " + path.string() + " IS RELATIVE! Resolved to: " + resolved.string();
    print_message(CLI_LEVEL_WARNING, msg);

    return resolved;
}

void capiocl::Parser::validate_json(nlohmann::json doc) {
    nlohmann::json schema_doc;
    valijson::Schema schema;
    valijson::SchemaParser schema_parser;
    valijson::Validator validator;

    std::ifstream schema_file("schema/v1.json");
    if (!schema_file.is_open()) {
        throw ParserException("Failed to open JSON schema!");
    }
    schema_file >> schema_doc;

    valijson::adapters::NlohmannJsonAdapter schema_adapter(schema_doc);
    schema_parser.populateSchema(schema_adapter, schema);

    valijson::adapters::NlohmannJsonAdapter target_adapter(doc);
    valijson::ValidationResults results;

    bool is_valid;

    try {
        is_valid = validator.validate(schema, target_adapter, &results);
    } catch (std::exception &e) {
        // If runtime exception is thrown, convert it to ParserException
        throw ParserException(e.what());
    }

    if (!is_valid) {
        valijson::ValidationResults::Error error;

        while (results.popError(error)) {
            std::stringstream err;
            for (auto c : error.context) {
                err << c;
            }
            err << ": " << error.description;

            print_message(CLI_LEVEL_ERROR, err.str());
        }
        throw ParserException("JSON validation failed!");
    }
}

std::tuple<std::string, capiocl::Engine *>
capiocl::Parser::parse(const std::filesystem::path &source,
                       const std::filesystem::path &resolve_prefix, bool store_only_in_memory) {

    std::string workflow_name = CAPIO_CL_DEFAULT_WF_NAME;
    auto engine               = new Engine();

    if (source.empty()) {
        throw ParserException("Empty source file name!");
    }

    // ---- Load JSON ----
    std::ifstream file(source);
    if (!file.is_open()) {
        throw ParserException("Failed to open file!");
    }

    nlohmann::json doc;
    file >> doc;

    validate_json(doc);

    // ---- workflow name ----
    workflow_name = doc["name"].get<std::string>();
    print_message(CLI_LEVEL_JSON, "Parsing configuration for workflow: " + workflow_name);

    // ---- IO_Graph ----
    for (const auto &app : doc["IO_Graph"]) {
        std::string app_name = app["name"].get<std::string>();
        print_message(CLI_LEVEL_JSON, "Parsing config for app " + app_name);

        // ---- input_stream ----
        print_message(CLI_LEVEL_JSON, "Parsing input_stream for app " + app_name);
        for (const auto &itm : app["input_stream"]) {
            auto file_path = resolve(itm.get<std::string>(), resolve_prefix);
            engine->newFile(file_path);
            engine->addConsumer(file_path, app_name);
        }

        // ---- output_stream ----
        print_message(CLI_LEVEL_JSON, "Parsing output_stream for app " + app_name);
        for (const auto &itm : app["output_stream"]) {
            auto file_path = resolve(itm.get<std::string>(), resolve_prefix);
            engine->newFile(file_path);
            engine->addProducer(file_path, app_name);
        }

        // ---- streaming ----
        if (app.contains("streaming") && app["streaming"].is_array()) {
            print_message(CLI_LEVEL_JSON, "Parsing streaming for app " + app_name);
            for (const auto &stream_item : app["streaming"]) {
                bool is_file = true;
                std::vector<std::filesystem::path> streaming_names;
                std::vector<std::filesystem::path> file_deps;
                std::string commit_rule = commit_rules::ON_TERMINATION;
                std::string mode        = fire_rules::UPDATE;
                long int n_close        = 0;
                int64_t n_files         = 0;

                // name or dirname
                if (stream_item.contains("name") && stream_item["name"].is_array()) {
                    for (const auto &nm : stream_item["name"]) {
                        auto nm_resolved = resolve(nm.get<std::string>(), resolve_prefix);
                        streaming_names.push_back(nm_resolved);
                    }
                } else if (stream_item.contains("dirname") && stream_item["dirname"].is_array()) {
                    is_file = false;
                    for (const auto &nm : stream_item["dirname"]) {
                        auto nm_resolved = resolve(nm.get<std::string>(), resolve_prefix);
                        streaming_names.push_back(nm_resolved);
                    }
                }

                // Commit rule. Optional in nature, hence no check required!
                if (stream_item.contains("committed")) {

                    std::string committed = stream_item["committed"].get<std::string>();
                    auto pos              = committed.find(':');
                    if (pos != std::string::npos) {
                        commit_rule           = committed.substr(0, pos);
                        std::string count_str = committed.substr(pos + 1);
                        try {
                            size_t num_len;
                            std::stoi(count_str, &num_len);
                        } catch (...) {
                            auto msg = "commit rule argument is not an integer!";
                            throw ParserException(msg);
                        }

                        if (commit_rule == commit_rules::ON_CLOSE) {
                            n_close = std::stol(count_str);
                        }
                    } else {
                        commit_rule = committed;
                    }

                    // file_deps
                    if (commit_rule == commit_rules::ON_FILE) {
                        for (const auto &dep : stream_item["file_deps"]) {
                            auto dep_resolved = resolve(dep.get<std::string>(), resolve_prefix);
                            file_deps.push_back(dep_resolved);
                        }
                    }
                }

                // Firing rule. Optional in nature, hence no check required!
                if (stream_item.contains("mode")) {
                    mode = stream_item["mode"].get<std::string>();
                }

                // n_files (optional)
                if (stream_item.contains("n_files") && !is_file) {
                    n_files = stream_item["n_files"].get<int64_t>();
                }

                for (auto &path : streaming_names) {
                    if (n_files != 0) {
                        engine->setDirectoryFileCount(path, n_files);
                    }
                    if (is_file) {
                        engine->setFile(path);
                    } else {
                        engine->setDirectory(path);
                    }

                    print_message(CLI_LEVEL_INFO,
                                  "App: " + app_name + " - " + "path: " + path.string() + " - " +
                                      "committed: " + commit_rule + " - " + "mode: " + mode +
                                      " - " + "n_files: " + std::to_string(n_files) + " - " +
                                      "n_close: " + std::to_string(n_close));
                    print_message(CLI_LEVEL_INFO, "");

                    engine->setCommitRule(path, commit_rule);
                    engine->setFireRule(path, mode);
                    engine->setCommitedCloseNumber(path, n_close);
                    engine->setFileDeps(path, file_deps);
                }
            }
        }
    }

    // ---- permanent ----
    if (doc.contains("permanent") && doc["permanent"].is_array()) {
        for (const auto &item : doc["permanent"]) {
            std::filesystem::path path(item.get<std::string>());
            if (path.is_relative()) {
                path = resolve_prefix / path;
            }
            engine->newFile(path);
            engine->setPermanent(path, true);
        }
    }

    // ---- exclude ----
    if (doc.contains("exclude") && doc["exclude"].is_array()) {
        for (const auto &item : doc["exclude"]) {
            std::filesystem::path path(item.get<std::string>());
            if (path.is_relative()) {
                path = resolve_prefix / path;
            }
            engine->newFile(path);
            engine->setExclude(path, true);
        }
    }

    // ---- storage ----
    if (doc.contains("storage")) {
        if (!doc["storage"].is_object()) {
            throw ParserException("Wrong data type for storage section!");
        }
        const auto &storage = doc["storage"];

        if (storage.contains("memory")) {
            if (!storage["memory"].is_array()) {
                throw ParserException("Wrong data type for memory storage section");
            }
            for (const auto &f : storage["memory"]) {
                auto file_str = f.get<std::string>();
                engine->setStoreFileInMemory(file_str);
            }
        }

        if (storage.contains("fs")) {
            if (!storage["fs"].is_array()) {
                throw ParserException("Wrong data type for fs storage section");
            }
            for (const auto &f : storage["fs"]) {
                auto file_str = f.get<std::string>();
                engine->setStoreFileInFileSystem(file_str);
            }
        }
    }

    // ---- Store only in memory ----

    if (store_only_in_memory) {
        print_message(CLI_LEVEL_INFO, "Storing all files in memory");
        engine->setAllStoreInMemory();
    }

    return {workflow_name, engine};
}
