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

    jsoncons::json doc = jsoncons::json::parse(file);
    validate_json(doc);

    // ---- workflow name ----
    workflow_name = doc["name"].as<std::string>();
    print_message(CLI_LEVEL_JSON, "Parsing configuration for workflow: " + workflow_name);

    // ---- IO_Graph ----
    for (const auto &app : doc["IO_Graph"].array_range()) {
        std::string app_name = app["name"].as<std::string>();
        print_message(CLI_LEVEL_JSON, "Parsing config for app " + app_name);

        // ---- input_stream ----
        print_message(CLI_LEVEL_JSON, "Parsing input_stream for app " + app_name);
        for (const auto &itm : app["input_stream"].array_range()) {
            auto file_path = resolve(itm.as<std::string>(), resolve_prefix);
            engine->newFile(file_path);
            engine->addConsumer(file_path, app_name);
        }

        // ---- output_stream ----
        print_message(CLI_LEVEL_JSON, "Parsing output_stream for app " + app_name);
        for (const auto &itm : app["output_stream"].array_range()) {
            auto file_path = resolve(itm.as<std::string>(), resolve_prefix);
            engine->newFile(file_path);
            engine->addProducer(file_path, app_name);
        }

        // ---- streaming ----
        if (app.contains("streaming")) {
            print_message(CLI_LEVEL_JSON, "Parsing streaming for app " + app_name);
            for (const auto &stream_item : app["streaming"].array_range()) {
                bool is_file = true;
                std::vector<std::filesystem::path> streaming_names;
                std::vector<std::filesystem::path> file_deps;
                std::string commit_rule = commit_rules::ON_TERMINATION;
                std::string mode        = fire_rules::UPDATE;
                long int n_close        = 0;
                int64_t n_files         = 0;

                // name or dirname
                if (stream_item.contains("name")) {
                    for (const auto &nm : stream_item["name"].array_range()) {
                        auto nm_resolved = resolve(nm.as<std::string>(), resolve_prefix);
                        streaming_names.push_back(nm_resolved);
                    }
                } else if (stream_item.contains("dirname")) {
                    is_file = false;
                    for (const auto &nm : stream_item["dirname"].array_range()) {
                        auto nm_resolved = resolve(nm.as<std::string>(), resolve_prefix);
                        streaming_names.push_back(nm_resolved);
                    }
                }

                // Commit rule (optional)
                if (stream_item.contains("committed")) {
                    std::string committed = stream_item["committed"].as<std::string>();
                    auto pos              = committed.find(':');
                    if (pos != std::string::npos) {
                        commit_rule           = committed.substr(0, pos);
                        std::string count_str = committed.substr(pos + 1);
                        try {
                            size_t num_len;
                            std::stoi(count_str, &num_len);
                        } catch (...) {
                            throw ParserException("commit rule argument is not an integer!");
                        }

                        if (commit_rule == commit_rules::ON_CLOSE) {
                            n_close = std::stol(count_str);
                        }
                    } else {
                        commit_rule = committed;
                    }

                    if (commit_rule == commit_rules::ON_FILE) {
                        for (const auto &dep : stream_item["file_deps"].array_range()) {
                            auto dep_resolved = resolve(dep.as<std::string>(), resolve_prefix);
                            file_deps.push_back(dep_resolved);
                        }
                    }
                }

                // Firing rule (optional)
                if (stream_item.contains("mode")) {
                    mode = stream_item["mode"].as<std::string>();
                }

                // n_files (optional)
                if (stream_item.contains("n_files") && !is_file) {
                    n_files = stream_item["n_files"].as<int64_t>();
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

                    engine->setCommitRule(path, commit_rule);
                    engine->setFireRule(path, mode);
                    engine->setCommitedCloseNumber(path, n_close);
                    engine->setFileDeps(path, file_deps);
                }
            }
        }
    }

    // ---- permanent ----
    if (doc.contains("permanent")) {
        for (const auto &item : doc["permanent"].array_range()) {
            std::filesystem::path path(item.as<std::string>());
            if (path.is_relative()) {
                path = resolve_prefix / path;
            }
            engine->newFile(path);
            engine->setPermanent(path, true);
        }
    }

    // ---- exclude ----
    if (doc.contains("exclude")) {
        for (const auto &item : doc["exclude"].array_range()) {
            std::filesystem::path path(item.as<std::string>());
            if (path.is_relative()) {
                path = resolve_prefix / path;
            }
            engine->newFile(path);
            engine->setExclude(path, true);
        }
    }

    // ---- storage ----
    if (doc.contains("storage")) {
        const auto &storage = doc["storage"];

        if (storage.contains("memory")) {
            for (const auto &f : storage["memory"].array_range()) {
                std::string file_str = f.as<std::string>();
                engine->setStoreFileInMemory(file_str);
            }
        }

        if (storage.contains("fs")) {
            for (const auto &f : storage["fs"].array_range()) {
                std::string file_str = f.as<std::string>();
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
