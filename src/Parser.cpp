#include "capiocl.hpp"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

bool capiocl::Parser::isInteger(const std::string &s) {
    START_LOG(gettid(), "call(%s)", s.c_str());
    bool res = false;
    if (!s.empty()) {
        char *p;
        strtol(s.c_str(), &p, 10);
        res = *p == 0;
    }
    return res;
};

bool capiocl::Parser::firstIsSubpathOfSecond(const std::filesystem::path &path,
                                             const std::filesystem::path &base) {
    const auto mismatch_pair = std::mismatch(path.begin(), path.end(), base.begin(), base.end());
    return mismatch_pair.second == base.end();
}

std::tuple<std::string, capiocl::Engine *>
capiocl::Parser::parse(const std::filesystem::path &source, std::filesystem::path &resolve_prefix,
                       bool store_only_in_memory) {

    bool skip_resolve         = resolve_prefix.empty();
    std::string workflow_name = CAPIO_CL_DEFAULT_WF_NAME;
    auto locations            = new Engine();
    START_LOG(gettid(), "call(config_file='%s')", source.c_str());

    if (source.empty()) {
        return {workflow_name, locations};
    }

    // ---- Load JSON ----
    std::ifstream file(source);
    if (!file.is_open()) {
        std::string msg = "Failed to open config file: " + source.string();
        print_message(CLI_LEVEL_ERROR, msg);
        ERR_EXIT(msg.c_str());
    }

    nlohmann::json doc;
    try {
        file >> doc;
    } catch (const std::exception &e) {
        std::string err = "JSON parse error: ";
        err += e.what();
        print_message(CLI_LEVEL_ERROR, err);
        ERR_EXIT(err.c_str());
    }

    // ---- workflow name ----
    if (!doc.contains("name") || !doc["name"].is_string()) {
        print_message(CLI_LEVEL_ERROR, "Missing workflow name!");
        ERR_EXIT("Error: workflow name is mandatory");
    }

    workflow_name = doc["name"].get<std::string>();
    print_message(CLI_LEVEL_JSON, "Parsing configuration for workflow: " + workflow_name);
    LOG("Parsing configuration for workflow: %s", workflow_name.c_str());

    // ---- IO_Graph ----
    if (!doc.contains("IO_Graph") || !doc["IO_Graph"].is_array()) {
        ERR_EXIT("Error: IO_Graph section missing or invalid");
    }

    for (const auto &app : doc["IO_Graph"]) {
        if (!app.contains("name") || !app["name"].is_string()) {
            print_message(CLI_LEVEL_ERROR, "Missing IO_Graph name or name is not a valid string!");
            ERR_EXIT("Error: app name is mandatory");
        }

        std::string app_name = app["name"].get<std::string>();
        print_message(CLI_LEVEL_JSON, "Parsing config for app " + app_name);
        LOG("Parsing config for app %s", app_name.c_str());

        // ---- input_stream ----
        if (!app.contains("input_stream") || !app["input_stream"].is_array()) {
            std::string msg = "No input_stream section found for app " + app_name;
            print_message(CLI_LEVEL_ERROR, msg);
            ERR_EXIT(msg.c_str());
        }

        print_message(CLI_LEVEL_JSON, "Parsing input_stream for app " + app_name);
        for (const auto &itm : app["input_stream"]) {
            std::filesystem::path file_path(itm.get<std::string>());
            if (file_path.is_relative() && !skip_resolve) {
                print_message(CLI_LEVEL_WARNING,
                              "Path : " + file_path.string() + " IS RELATIVE! resolving...");
                file_path = resolve_prefix / file_path;
            }
            locations->newFile(file_path);
            locations->addConsumer(file_path, app_name);
        }

        // ---- output_stream ----
        if (!app.contains("output_stream") || !app["output_stream"].is_array()) {
            std::string msg = "No output_stream section found for app " + app_name;
            print_message(CLI_LEVEL_ERROR, msg);
            ERR_EXIT(msg.c_str());
        }
        print_message(CLI_LEVEL_JSON, "Parsing output_stream for app " + app_name);
        for (const auto &itm : app["output_stream"]) {
            std::filesystem::path file_path(itm.get<std::string>());
            if (file_path.is_relative() && !skip_resolve) {
                print_message(CLI_LEVEL_WARNING,
                              "Path : " + file_path.string() + " IS RELATIVE! resolving...");
                file_path = resolve_prefix / file_path;
            }
            locations->newFile(file_path);
            locations->addProducer(file_path, app_name);
        }

        // ---- streaming ----
        if (app.contains("streaming") && app["streaming"].is_array()) {
            print_message(CLI_LEVEL_JSON, "Parsing streaming for app " + app_name);
            for (const auto &stream_item : app["streaming"]) {
                bool is_file = true;
                std::vector<std::filesystem::path> streaming_names;
                std::vector<std::string> file_deps;
                std::string commit_rule = COMMITTED_ON_TERMINATION, mode = MODE_UPDATE;
                long int n_close = 0;
                int64_t n_files  = 0;

                // name or dirname
                if (stream_item.contains("name") && stream_item["name"].is_array()) {
                    for (const auto &nm : stream_item["name"]) {
                        std::filesystem::path p(nm.get<std::string>());
                        if (p.is_relative() && !skip_resolve) {
                            p = resolve_prefix / p;
                        }
                        streaming_names.push_back(p);
                    }
                } else if (stream_item.contains("dirname") && stream_item["dirname"].is_array()) {
                    is_file = false;
                    for (const auto &nm : stream_item["dirname"]) {
                        std::filesystem::path p(nm.get<std::string>());
                        if (p.is_relative() && !skip_resolve) {
                            p = resolve_prefix / p;
                        }
                        streaming_names.push_back(p);
                    }
                } else {
                    print_message(
                        CLI_LEVEL_ERROR,
                        "Missing streaming name/dirname, or name/dirname is not an array for app " +
                            app_name);
                    ERR_EXIT("error: either name or dirname in streaming section is required");
                }

                // Commit rule. Optional in nature, hence no check required!
                if (stream_item.contains("committed")) {
                    if (!stream_item["committed"].is_string()) {
                        print_message(CLI_LEVEL_ERROR, "Error: invalid type for commit rule!");
                        ERR_EXIT("Error: invalid type for commit rule!");
                    }

                    std::string committed = stream_item["committed"].get<std::string>();
                    auto pos              = committed.find(':');
                    if (pos != std::string::npos) {
                        commit_rule           = committed.substr(0, pos);
                        std::string count_str = committed.substr(pos + 1);
                        if (!isInteger(count_str)) {
                            ERR_EXIT("invalid number in commit rule");
                        }
                        if (commit_rule == COMMITTED_ON_CLOSE) {
                            n_close = std::stol(count_str);
                        } else if (commit_rule == COMMITTED_N_FILES) {
                            n_files = std::stol(count_str);
                        } else {
                            ERR_EXIT("invalid commit rule type");
                        }
                    } else {
                        commit_rule = committed;
                    }

                    // file_deps
                    if (commit_rule == COMMITTED_ON_FILE) {
                        if (!stream_item.contains("file_deps") ||
                            !stream_item["file_deps"].is_array()) {
                            ERR_EXIT("commit rule is on_file but no file_deps section found");
                        }
                        for (const auto &dep : stream_item["file_deps"]) {
                            std::filesystem::path p(dep.get<std::string>());
                            if (p.is_relative() && !skip_resolve) {
                                p = resolve_prefix / p;
                            }
                            file_deps.push_back(p);
                        }
                    }

                    // check commit rule is one of the available
                    if (commit_rule != capiocl::COMMITTED_N_FILES &&
                        commit_rule != capiocl::COMMITTED_ON_CLOSE &&
                        commit_rule != capiocl::COMMITTED_ON_FILE &&
                        commit_rule != capiocl::COMMITTED_ON_TERMINATION) {
                        print_message(CLI_LEVEL_ERROR, "Error: commit rule " + commit_rule +
                                                           " is not one of the allowed one!");
                        ERR_EXIT("Unknown commit rule %s", commit_rule.c_str());
                    }
                }

                // Firing rule. Optional in nature, hence no check required!
                if (stream_item.contains("mode")) {
                    if (!stream_item["mode"].is_string()) {
                        print_message(CLI_LEVEL_ERROR, "Error: invalid type for mode");
                        ERR_EXIT("Error: invalid type for mode");
                    }
                    mode = stream_item["mode"].get<std::string>();

                    if (mode != capiocl::MODE_UPDATE && mode != capiocl::MODE_NO_UPDATE) {
                        print_message(CLI_LEVEL_ERROR,
                                      "Error: invalid firing rule provided for app: " + app_name);
                        ERR_EXIT("Error: invalid firing rule provided for app: %s",
                                 app_name.c_str());
                    }
                }

                // n_files (optional)
                if (stream_item.contains("n_files") && stream_item["n_files"].is_number_integer()) {
                    n_files = stream_item["n_files"].get<int64_t>();
                }

                for (auto &path : streaming_names) {
                    if (n_files != -1) {
                        locations->setDirectoryFileCount(path, n_files);
                    }
                    if (is_file) {
                        locations->setFile(path);
                    } else {
                        locations->setDirectory(path);
                    }

                    print_message(CLI_LEVEL_INFO,
                                  "App: " + app_name + " - " + "path: " + path.string() + " - " +
                                      "committed: " + commit_rule + " - " + "mode: " + mode +
                                      " - " + "n_files: " + std::to_string(n_files) + " - " +
                                      "n_close: " + std::to_string(n_close));
                    print_message(CLI_LEVEL_INFO, "");

                    locations->setCommitRule(path, commit_rule);
                    locations->setFireRule(path, mode);
                    locations->setCommitedCloseNumber(path, n_close);
                    locations->setFileDeps(path, file_deps);
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
            locations->newFile(path);
            locations->setPermanent(path, true);
        }
    }

    // ---- exclude ----
    if (doc.contains("exclude") && doc["exclude"].is_array()) {
        for (const auto &item : doc["exclude"]) {
            std::filesystem::path path(item.get<std::string>());
            if (path.is_relative()) {
                path = resolve_prefix / path;
            }
            locations->newFile(path);
            locations->setExclude(path, true);
        }
    }

    // ---- storage ----
    if (doc.contains("storage") && doc["storage"].is_object()) {
        const auto &storage = doc["storage"];

        if (storage.contains("memory") && storage["memory"].is_array()) {
            for (const auto &f : storage["memory"]) {
                auto file_str = f.get<std::string>();
                locations->setStoreFileInMemory(file_str);
            }
        }

        if (storage.contains("fs") && storage["fs"].is_array()) {
            for (const auto &f : storage["fs"]) {
                auto file_str = f.get<std::string>();
                locations->setStoreFileInFileSystem(file_str);
            }
        }
    }

    // ---- Store only in memory ----

    if (store_only_in_memory) {
        print_message(CLI_LEVEL_INFO, "Storing all files in memory");
        locations->setAllStoreInMemory();
    }

    return {workflow_name, locations};
}
