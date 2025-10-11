#include "capiocl.hpp"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

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

    // ---- workflow name ----
    if (!doc.contains("name")) {
        throw ParserException("Missing workflow name!");
    }
    if (!doc["name"].is_string()) {
        throw ParserException("Wrong data type for workflow name!");
    }

    workflow_name = doc["name"].get<std::string>();
    print_message(CLI_LEVEL_JSON, "Parsing configuration for workflow: " + workflow_name);

    // ---- IO_Graph ----
    if (!doc.contains("IO_Graph")) {
        throw ParserException("Missing IO_Graph section");
    }
    if (!doc["IO_Graph"].is_array()) {
        throw ParserException("Wrong data type for IO_Graph section");
    }

    for (const auto &app : doc["IO_Graph"]) {
        if (!app.contains("name")) {
            throw ParserException("Missing name for streaming item!");
        }
        if (!app["name"].is_string()) {
            throw ParserException("Wrong type for name streaming entry!");
        }

        std::string app_name = app["name"].get<std::string>();
        print_message(CLI_LEVEL_JSON, "Parsing config for app " + app_name);

        // ---- input_stream ----
        if (!app.contains("input_stream")) {
            throw ParserException("No input_stream section found for app " + app_name);
        }
        if (!app["input_stream"].is_array()) {
            throw ParserException("input_stream section for app " + app_name + " is not array!");
        }

        print_message(CLI_LEVEL_JSON, "Parsing input_stream for app " + app_name);
        for (const auto &itm : app["input_stream"]) {
            auto file_path = resolve(itm.get<std::string>(), resolve_prefix);
            engine->newFile(file_path);
            engine->addConsumer(file_path, app_name);
        }

        // ---- output_stream ----
        if (!app.contains("output_stream")) {
            throw ParserException("No output_stream section found for app " + app_name);
        }
        if (!app["output_stream"].is_array()) {
            throw ParserException("output_stream section for app " + app_name + " is not array!");
        }

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
                } else {
                    throw ParserException(
                        "Missing streaming name/dirname, or name/dirname is not an array for app " +
                        app_name);
                }

                // Commit rule. Optional in nature, hence no check required!
                if (stream_item.contains("committed")) {
                    if (!stream_item["committed"].is_string()) {
                        throw ParserException("Error: invalid type for commit rule!");
                    }

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
                        } else if (commit_rule == commit_rules::N_FILES) {
                            n_files = std::stol(count_str);
                        } else {
                            throw ParserException("Invalid commit rule!");
                        }
                    } else {
                        commit_rule = committed;
                    }

                    // file_deps
                    if (commit_rule == commit_rules::ON_FILE) {
                        if (!stream_item.contains("file_deps") ||
                            !stream_item["file_deps"].is_array()) {
                            throw ParserException(
                                "commit rule is on_file but no file_deps section found");
                        }
                        for (const auto &dep : stream_item["file_deps"]) {
                            auto dep_resolved = resolve(dep.get<std::string>(), resolve_prefix);
                            file_deps.push_back(dep_resolved);
                        }
                    }

                    // check commit rule is one of the available
                    if (commit_rule != commit_rules::N_FILES &&
                        commit_rule != commit_rules::ON_CLOSE &&
                        commit_rule != commit_rules::ON_FILE &&
                        commit_rule != commit_rules::ON_TERMINATION) {
                        throw ParserException("Error: commit rule " + commit_rule +
                                              " is not one of the allowed one!");
                    }
                }

                // Firing rule. Optional in nature, hence no check required!
                if (stream_item.contains("mode")) {
                    if (!stream_item["mode"].is_string()) {
                        throw ParserException("Error: invalid firing rule data type");
                    }
                    mode = stream_item["mode"].get<std::string>();

                    if (mode != fire_rules::UPDATE && mode != fire_rules::NO_UPDATE) {
                        throw ParserException(
                            "Error: fire rule is not one of the allowed ones for app: " + app_name);
                    }
                }

                // n_files (optional)
                if (stream_item.contains("n_files")) {
                    if (!stream_item["n_files"].is_number_integer()) {
                        throw ParserException("wrong type for n_files!");
                    }
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
