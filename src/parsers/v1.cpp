#include <fstream>

#include "capio_cl_json_schemas.hpp"
#include "capiocl.hpp"
#include "capiocl/engine.h"
#include "capiocl/parser.h"
#include "capiocl/printer.h"

capiocl::engine::Engine *
capiocl::parser::Parser::available_parsers::parse_v1(const std::filesystem::path &source,
                                                     const std::filesystem::path &resolve_prefix,
                                                     bool store_only_in_memory) {
    std::string workflow_name = CAPIO_CL_DEFAULT_WF_NAME;
    auto engine               = new engine::Engine(true);

    engine->useDefaultConfiguration();

    // ---- Load JSON ----
    std::ifstream file(source);

    jsoncons::json doc = jsoncons::json::parse(file);
    validate_json(doc, schema_v1);

    // ---- workflow name ----
    workflow_name = doc["name"].as<std::string>();
    engine->setWorkflowName(workflow_name);
    printer::print(printer::CLI_LEVEL_JSON, "Parsing configuration for workflow: " + workflow_name);

    // ---- IO_Graph ----
    for (const auto &app : doc["IO_Graph"].array_range()) {
        std::string app_name = app["name"].as<std::string>();
        printer::print(printer::CLI_LEVEL_JSON, "Parsing config for app " + app_name);

        // ---- input_stream ----
        printer::print(printer::CLI_LEVEL_JSON, "Parsing input_stream for app " + app_name);
        for (const auto &itm : app["input_stream"].array_range()) {
            auto file_path = resolve(itm.as<std::string>(), resolve_prefix);
            engine->newFile(file_path);
            engine->addConsumer(file_path, app_name);
        }

        // ---- output_stream ----
        printer::print(printer::CLI_LEVEL_JSON, "Parsing output_stream for app " + app_name);
        for (const auto &itm : app["output_stream"].array_range()) {
            auto file_path = resolve(itm.as<std::string>(), resolve_prefix);
            engine->newFile(file_path);
            engine->addProducer(file_path, app_name);
        }

        // ---- streaming ----
        if (app.contains("streaming")) {
            printer::print(printer::CLI_LEVEL_JSON, "Parsing streaming for app " + app_name);
            for (const auto &stream_item : app["streaming"].array_range()) {
                bool is_file = true;
                std::vector<std::filesystem::path> streaming_names;
                std::vector<std::filesystem::path> file_deps;
                std::string commit_rule;
                std::string fire_rule;
                long int n_close = 0;
                int64_t n_files  = 0;

                if (stream_item.contains("name")) {
                    for (const auto &nm : stream_item["name"].array_range()) {
                        auto nm_resolved = resolve(nm.as<std::string>(), resolve_prefix);
                        streaming_names.push_back(nm_resolved);
                    }
                } else {
                    // At this point we have dirname, as either name or dirname is required
                    // This is checked by the JSON schema validation phase
                    is_file = false;
                    for (const auto &nm : stream_item["dirname"].array_range()) {
                        auto nm_resolved = resolve(nm.as<std::string>(), resolve_prefix);
                        streaming_names.push_back(nm_resolved);
                    }
                }

                // Commit rule (optional)
                if (stream_item.contains("committed")) {
                    auto committed = stream_item["committed"].as<std::string>();
                    auto pos       = committed.find(':');
                    if (pos != std::string::npos) {
                        // If we reach here, we are certain that the commit rule is on_close
                        // as the json schema enforces the rule that the :n is allowed only for
                        // the on_close commit rule.

                        size_t num_len;
                        std::string count_str = committed.substr(pos + 1);
                        n_close               = std::stoi(count_str, &num_len);

                        // clean up committed
                        committed = committed.substr(0, pos);
                    }

                    commit_rule = committed;

                    if (commit_rule == commitRules::ON_FILE) {
                        for (const auto &dep : stream_item["file_deps"].array_range()) {
                            auto dep_resolved = resolve(dep.as<std::string>(), resolve_prefix);
                            file_deps.push_back(dep_resolved);
                        }
                    }
                } else {
                    commit_rule = commitRules::ON_TERMINATION;
                }

                // Firing rule (optional)
                if (stream_item.contains("mode")) {
                    fire_rule = stream_item["mode"].as<std::string>();
                } else {
                    fire_rule = fireRules::NO_UPDATE;
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
                    engine->setFireRule(path, fire_rule);
                    engine->setCommitedCloseNumber(path, n_close);
                    engine->setFileDeps(path, file_deps);
                }
            }
        }
    }

    // ---- permanent ----
    if (doc.contains("permanent")) {
        for (const auto &item : doc["permanent"].array_range()) {
            std::filesystem::path path = resolve(item.as<std::string>(), resolve_prefix);
            engine->newFile(path);
            engine->setPermanent(path, true);
        }
    }

    // ---- exclude ----
    if (doc.contains("exclude")) {
        for (const auto &item : doc["exclude"].array_range()) {
            std::filesystem::path path = resolve(item.as<std::string>(), resolve_prefix);
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
        } else {
            printer::print(printer::CLI_LEVEL_INFO, "No MEM storage section found");
        }

        if (storage.contains("fs")) {
            for (const auto &f : storage["fs"].array_range()) {
                std::string file_str = f.as<std::string>();
                engine->setStoreFileInFileSystem(file_str);
            }
        } else {
            printer::print(printer::CLI_LEVEL_INFO, "No FS storage section found");
        }
    } else {
        printer::print(printer::CLI_LEVEL_INFO, "No storage section found");
    }

    // ---- Store only in memory ----
    if (store_only_in_memory) {
        printer::print(printer::CLI_LEVEL_INFO, "Storing all files in memory");
        engine->setAllStoreInMemory();
    }

    return engine;
}