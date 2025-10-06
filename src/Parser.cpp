#include "capiocl.hpp"
#include "singleheader/simdjson.h"
#include <filesystem>

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
capiocl::Parser::parse(const std::filesystem::path &source,
                       const std::filesystem::path &resolve_prexix, bool store_only_in_memory) {

    std::string workflow_name = CAPIO_CL_DEFAULT_WF_NAME;
    auto locations            = new Engine();
    START_LOG(gettid(), "call(config_file='%s')", source.c_str());

    /*
     * Before here a call to get_capio_dir() was issued. However, to support multiple CAPIO_DIRs
     * there is no difference to use the wildcard * instead of CAPIO_DIR. This is true as only
     * paths relative to the capio_dir directory are forwarded to the server, and as such, there
     * is no difference that to create a ROOT dir equal to CAPIO_DIR compared to the wildcard *.
     */
    locations->newFile("*");
    locations->setDirectory("*");
    if (store_only_in_memory) {
        locations->setStoreFileInMemory("*");
    }

    if (source.empty()) {
        return {workflow_name, locations};
    }

    simdjson::ondemand::parser parser;
    simdjson::padded_string json;
    simdjson::ondemand::document entries;
    simdjson::ondemand::array input_stream, output_stream, streaming, permanent_files,
        exclude_files, storage_memory, storage_fs;
    simdjson::ondemand::object storage_section;
    simdjson::error_code error;

    try {
        json = simdjson::padded_string::load(source.c_str());
    } catch (const simdjson::simdjson_error &e) {

        const std::string message_line =
            std::string("Exception thrown while opening config file:") + e.what();
        print_message(CLI_LEVEL_ERROR, message_line);
        LOG(errmessg.c_str());
        ERR_EXIT(errmessg.c_str());
    }

    entries = parser.iterate(json);
    std::string_view wf_name;
    error = entries["name"].get_string().get(wf_name);
    if (error) {
        ERR_EXIT("Error: workflow name is mandatory");
    }
    workflow_name = std::string(wf_name);

    print_message(CLI_LEVEL_JSON, "Parsing configuration for workflow: " + workflow_name);

    LOG("Parsing configuration for workflow: %s", std::string(workflow_name).c_str());

    print_message(CLI_LEVEL_JSON, "");

    auto io_graph = entries["IO_Graph"];

    for (auto app : io_graph) {
        std::string_view app_name;
        error = app["name"].get_string().get(app_name);
        if (error) {
            ERR_EXIT("Error: app name is mandatory");
        }

        print_message(CLI_LEVEL_JSON, "Parsing config for app " + std::string(app_name));
        LOG("Parsing config for app %s", std::string(app_name).c_str());

        if (app["input_stream"].get_array().get(input_stream)) {
            print_message(CLI_LEVEL_ERROR,
                          "No input_stream section found for app " + std::string(app_name));
            ERR_EXIT("No input_stream section found for app %s", std::string(app_name).c_str());
        }
        for (auto itm : input_stream) {
            std::filesystem::path file(itm.get_string().take_value());

            print_message(CLI_LEVEL_JSON, "Found file " + std::string(file));

            if (file.is_relative()) {

                print_message(CLI_LEVEL_WARNING,
                              "Path : " + std::string(file) +
                                  " IS RELATIVE! using cwd() of server to compute abs path.");
                file = resolve_prexix / file;
            }
            std::string appname(app_name);

            print_message(CLI_LEVEL_JSON, "Path : " + std::string(file) +
                                              " added to app: " + std::string(app_name));

            locations->newFile(file.c_str());
            locations->addConsumer(file, appname);
        }

        print_message(CLI_LEVEL_JSON,
                      "Completed input_stream parsing for app: " + std::string(app_name));

        LOG("Completed input_stream parsing for app: %s", std::string(app_name).c_str());

        if (app["output_stream"].get_array().get(output_stream)) {

            print_message(CLI_LEVEL_ERROR,
                          "No output_stream section found for app " + std::string(app_name));
            ERR_EXIT("No output_stream section found for app %s", std::string(app_name).c_str());
        }
        for (auto itm : output_stream) {
            std::filesystem::path file(itm.get_string().take_value());
            if (file.is_relative()) {
                if (file.is_relative()) {
                    print_message(CLI_LEVEL_WARNING,
                                  "Path : " + std::string(file) +
                                      " IS RELATIVE! using cwd() of server to compute abs path.");
                    file = resolve_prexix / file;
                }
            }
            std::string appname(app_name);

            print_message(CLI_LEVEL_JSON,
                          "Adding file: " + std::string(file) + " to app: " + appname);

            locations->newFile(file);
            locations->addProducer(file, appname);
        }

        print_message(CLI_LEVEL_JSON,
                      "Completed output_stream parsing for app: " + std::string(app_name));
        LOG("Completed output_stream parsing for app: %s", std::string(app_name).c_str());

        // PARSING STREAMING FILES
        if (app["streaming"].get_array().get(streaming)) {
            print_message(CLI_LEVEL_WARNING,
                          "No Streaming section found for app " + std::string(app_name));
            LOG("No streaming section found for app: %s", std::string(app_name).c_str());
        } else {
            LOG("Began parsing streaming section for app %s", std::string(app_name).c_str());
            for (auto file : streaming) {
                std::string_view committed, mode, commit_rule;
                std::vector<std::filesystem::path> streaming_names;
                std::vector<std::string> file_deps;
                long int n_close = -1;
                long n_files     = -1;
                bool is_file     = true;

                simdjson::ondemand::array name;
                error = file["name"].get_array().get(name);
                if (error || name.is_empty()) {
                    error = file["dirname"].get_array().get(name);
                    if (error || name.is_empty()) {
                        print_message(
                            CLI_LEVEL_ERROR,
                            "error: either name or dirname in streaming section is required");
                        ERR_EXIT("error: either name or dirname in streaming section is required");
                    }
                    is_file = false;
                }

                for (auto item : name) {
                    std::string_view elem = item.get_string().value();
                    LOG("Found name: %s", std::string(elem).c_str());
                    std::filesystem::path file_fs(elem);
                    if (file_fs.is_relative()) {
                        print_message(
                            CLI_LEVEL_WARNING,
                            "Path : " + std::string(file_fs) +
                                " IS RELATIVE! using cwd() of server to compute abs path.");
                        file_fs = resolve_prexix / file_fs;
                    }
                    LOG("Saving file %s to locations", std::string(elem).c_str());
                    streaming_names.emplace_back(file_fs);
                }

                // PARSING COMMITTED
                error = file["committed"].get_string().get(committed);
                if (error) {
                    print_message(CLI_LEVEL_ERROR, "commit rule is mandatory in streaming section");
                    ERR_EXIT("error commit rule is mandatory in streaming section");
                }
                auto pos = committed.find(':');
                if (pos != std::string::npos) {
                    commit_rule = committed.substr(0, pos);
                    std::string count_str(committed.substr(pos + 1, committed.length()));
                    if (!isInteger(count_str)) {
                        print_message(CLI_LEVEL_ERROR,
                                      "commit rule on_close/n_files invalid number");
                        ERR_EXIT("error commit rule on_close invalid number: !is_int()");
                    }

                    if (commit_rule == COMMITTED_ON_CLOSE) {
                        n_close = std::stol(count_str);
                    } else if (commit_rule == COMMITTED_N_FILES) {
                        n_files = std::stol(count_str);
                    } else {
                        print_message(CLI_LEVEL_ERROR,
                                      "Invalid commit rule: " + std::string(commit_rule));
                        ERR_EXIT("error commit rule: %s for operand ':'. Either on_close or "
                                 "n_files is expected",
                                 std::string(commit_rule).c_str());
                    }

                } else {
                    commit_rule = committed;
                }

                // check for committed on file:
                if (commit_rule == COMMITTED_ON_FILE) {
                    simdjson::ondemand::array file_deps_tmp;
                    error = file["file_deps"].get_array().get(file_deps_tmp);

                    if (error) {
                        print_message(CLI_LEVEL_ERROR,
                                      "commit rule is on_file but no file_deps section found");
                        ERR_EXIT("commit rule is on_file but no file_deps section found");
                    }

                    std::string_view name_tmp;
                    for (auto itm : file_deps_tmp) {
                        name_tmp = itm.get_string().value();
                        std::filesystem::path computed_path(name_tmp);
                        if (computed_path.is_relative()) {
                            print_message(
                                CLI_LEVEL_WARNING,
                                "Path : " + std::string(computed_path) +
                                    " IS RELATIVE! using cwd() of server to compute abs path.");
                            computed_path = resolve_prexix / computed_path;
                        }
                        print_message(CLI_LEVEL_JSON, "Adding file: " + std::string(computed_path) +
                                                          " to file dependencies: ");

                        file_deps.emplace_back(computed_path);
                    }
                }

                LOG("Committed: %s", std::string(committed).c_str());
                // END PARSING COMMITTED

                error = file["mode"].get_string().get(mode);
                if (error) {
                    mode = MODE_UPDATE;
                }
                LOG("Mode: %s", std::string(mode).c_str());

                if (n_files == -1) {
                    error = file["n_files"].get_int64().get(n_files);
                    if (error && n_files != -1) {
                        n_files = -1;
                    }
                }
                LOG("n_files: %d", n_files);

                for (auto path : streaming_names) {
                    print_message(CLI_LEVEL_JSON,
                                  "Updating metadata for path:  " + std::string(path));

                    if (path.is_relative()) {
                        print_message(
                            CLI_LEVEL_WARNING,
                            "Path : " + std::string(path) +
                                " IS RELATIVE! using cwd() of server to compute abs path.");
                        path = resolve_prexix / path;
                    }
                    LOG("path: %s", path.c_str());

                    if (n_files != -1) {

                        print_message(CLI_LEVEL_JSON, "Setting path:  " + std::string(path) +
                                                          " n_files to " + std::to_string(n_files));
                        locations->setDirectoryFileCount(path, n_files);
                    }

                    is_file ? locations->setFile(path) : locations->setDirectory(path);
                    locations->setCommitRule(path, commit_rule.data());
                    locations->setFireRule(path, mode.data());
                    locations->setCommitedCloseNumber(path, n_close);
                    locations->setFileDeps(path, file_deps);
                }
            }

            print_message(CLI_LEVEL_JSON, "completed parsing of streaming section for app: " +
                                              std::string(app_name));
            LOG("completed parsing of streaming section for app: %s",
                std::string(app_name).c_str());
        } // END PARSING STREAMING FILES
        print_message(CLI_LEVEL_JSON, "");
    } // END OF APP MAIN LOOPS
    LOG("Completed parsing of io_graph app main loops");

    print_message(CLI_LEVEL_JSON, "Completed parsing of io_graph");
    LOG("Completed parsing of io_graph");

    if (entries["permanent"].get_array().get(permanent_files)) {
        // PARSING PERMANENT FILES
        print_message(CLI_LEVEL_WARNING,
                      "No permanent section found for workflow: " + workflow_name);
        LOG("No permanent section found for workflow: %s", workflow_name.c_str());
    } else {
        for (auto file : permanent_files) {
            std::string_view name;
            error = file.get_string().get(name);
            if (error) {
                ERR_EXIT("error name for permanent section is mandatory");
            }
            LOG("Permanent name: %s", std::string(name).c_str());

            std::filesystem::path path(name);

            if (path.is_relative()) {
                print_message(CLI_LEVEL_WARNING,
                              "Path : " + std::string(path) +
                                  " IS RELATIVE! using cwd() of server to compute abs path.");
                path = resolve_prexix / path;
            }

            locations->setPermanent(path, true);
        }
        print_message(CLI_LEVEL_JSON, "Completed parsing of permanent files");
        LOG("Completed parsing of permanent files");
    } // END PARSING PERMANENT FILES

    print_message(CLI_LEVEL_JSON, "");

    if (entries["exclude"].get_array().get(exclude_files)) {
        // PARSING PERMANENT FILES
        print_message(CLI_LEVEL_WARNING, "No exclude section found for workflow: " + workflow_name);
        LOG("No exclude section found for workflow: %s", std::string(workflow_name).c_str());
    } else {
        for (auto file : exclude_files) {
            std::string_view name;
            error = file.get_string().get(name);
            if (error) {
                ERR_EXIT("error name for exclude section is mandatory");
            }
            LOG("exclude name: %s", std::string(name).c_str());

            std::filesystem::path path(name);

            if (path.is_relative()) {
                print_message(CLI_LEVEL_WARNING,
                              "Path : " + std::string(path) +
                                  " IS RELATIVE! using cwd() of server to compute abs path.");
                path = resolve_prexix / path;
            }
            locations->newFile(path);
            locations->setExclude(path, true);
        }

        print_message(CLI_LEVEL_JSON, "Completed parsing of exclude files");
        LOG("Completed parsing of exclude files");
    } // END PARSING PERMANENT FILES

    print_message(CLI_LEVEL_JSON, "");

    auto home_node_policies = entries["home_node_policy"].error();
    if (!home_node_policies) {
        print_message(CLI_LEVEL_WARNING,
                      "Warning: capio does not support home node policies yet! skipping section ");
    }

    if (entries["storage"].get_object().get(storage_section)) {
        print_message(CLI_LEVEL_WARNING, "No storage section found for workflow: " + workflow_name);
        LOG("No storage section found for workflow: %s", std::string(workflow_name).c_str());
    } else {
        if (storage_section["memory"].get_array().get(storage_memory)) {
            print_message(CLI_LEVEL_WARNING,
                          "No files listed in memory storage section for workflow: " +
                              workflow_name);
            LOG("No files listed in memory storage section for workflow: %s",
                std::string(workflow_name).c_str());
        } else {
            for (auto file : storage_memory) {
                if (std::string_view file_str; !file.get_string().get(file_str)) {
                    print_message(CLI_LEVEL_JSON, "Setting file " + std::string(file_str) +
                                                      " to be stored in memory");
                    locations->setStoreFileInMemory(file_str);
                } else {
                    print_message(CLI_LEVEL_WARNING, "Unbale to obtain file");
                }
            }
        }

        if (storage_section["fs"].get_array().get(storage_fs)) {
            print_message(CLI_LEVEL_WARNING,
                          "No files listed in fs storage section for workflow: " + workflow_name);
            LOG("No files listed in fs storage section for workflow: %s",
                std::string(workflow_name).c_str());
        } else {
            for (auto file : storage_fs) {
                if (std::string_view file_str; !file.get_string().get(file_str)) {
                    print_message(CLI_LEVEL_JSON, "Setting file " + std::string(file_str) +
                                                      " to be stored on file system");
                    locations->setStoreFileInFileSystem(file_str);
                } else {
                    print_message(CLI_LEVEL_WARNING, "Unbale to obtain file");
                }
            }
        }
        print_message(CLI_LEVEL_JSON, "Completed parsing of memory storage directives");
    }

    print_message(CLI_LEVEL_JSON, "");

    return {workflow_name, locations};
}