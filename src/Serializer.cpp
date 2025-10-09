#include "capiocl.hpp"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

void capiocl::Serializer::dump(const capiocl::Engine &engine, const std::string workflow_name,
                               const std::filesystem::path &filename) {
    START_LOG(gettid(), "call(output='%s')", target.c_str());

    nlohmann::json doc;
    doc["name"] = workflow_name;

    // Retrieve the files map
    const auto *files = engine.getLocations(); // adjust if it's a different getter

    // Build helper maps: app → inputs / outputs
    std::unordered_map<std::string, std::vector<std::string>> app_inputs;
    std::unordered_map<std::string, std::vector<std::string>> app_outputs;

    // For permanent/exclude/storage
    std::vector<std::string> permanent;
    std::vector<std::string> exclude;
    std::vector<std::string> memory_storage;
    std::vector<std::string> fs_storage;

    nlohmann::json io_graph = nlohmann::json::array();

    // We'll also need a mapping of each file → its metadata for streaming reconstruction
    for (const auto &[path, data] : *files) {
        const auto &[producers, consumers, commit_rule, fire_rule, permanent_flag, excluded_flag,
                     is_file, n_close, n_dir_files, file_deps, store_in_memory] = data;

        // Collect permanent/exclude info
        if (permanent_flag) {
            permanent.push_back(path);
        }
        if (excluded_flag) {
            exclude.push_back(path);
        }

        // Collect storage info
        if (store_in_memory) {
            memory_storage.push_back(path);
        } else {
            fs_storage.push_back(path);
        }

        // Collect app relationships
        for (const auto &p : producers) {
            app_outputs[p].push_back(path);
        }
        for (const auto &c : consumers) {
            app_inputs[c].push_back(path);
        }
    }

    // Construct IO_Graph section
    for (const auto &[app_name, outputs] : app_outputs) {
        nlohmann::json app;
        app["name"] = app_name;

        if (app_inputs.count(app_name)) {
            app["input_stream"] = app_inputs[app_name];
        } else {
            app["input_stream"] = nlohmann::json::array();
        }

        app["output_stream"] = outputs;

        // ---- streaming ----
        nlohmann::json streaming = nlohmann::json::array();

        for (const auto &path : outputs) {
            const auto &data = files->at(path);
            const auto &[producers, consumers, commit_rule, fire_rule, permanent_flag,
                         excluded_flag, is_file, n_close, n_dir_files, file_deps, store_in_fs] =
                data;

            nlohmann::json sitem;
            if (is_file) {
                sitem["name"] = nlohmann::json::array({path});
            } else {
                sitem["dirname"] = nlohmann::json::array({path});
            }

            // Commit rule
            std::string committed = commit_rule;
            if (commit_rule == capiocl::COMMITTED_ON_CLOSE && n_close > 0) {
                committed += ":" + std::to_string(n_close);
            } else if (commit_rule == capiocl::COMMITTED_N_FILES && n_dir_files > 0) {
                committed += ":" + std::to_string(n_dir_files);
            }

            sitem["committed"] = committed;

            // Mode / fire rule
            sitem["mode"] = fire_rule;

            // File dependencies (for COMMITTED_ON_FILE)
            if (commit_rule == capiocl::COMMITTED_ON_FILE && !file_deps.empty()) {
                sitem["file_deps"] = file_deps;
            }

            // Directory file count
            if (n_dir_files > 0) {
                sitem["n_files"] = n_dir_files;
            }

            streaming.push_back(sitem);
        }

        if (!streaming.empty()) {
            app["streaming"] = streaming;
        }

        io_graph.push_back(app);
    }

    doc["IO_Graph"] = io_graph;

    // ---- permanent / exclude ----
    if (!permanent.empty()) {
        doc["permanent"] = permanent;
    }
    if (!exclude.empty()) {
        doc["exclude"] = exclude;
    }

    // ---- storage ----
    nlohmann::json storage;
    if (!memory_storage.empty()) {
        storage["memory"] = memory_storage;
    }
    if (!fs_storage.empty()) {
        storage["fs"] = fs_storage;
    }
    if (!storage.empty()) {
        doc["storage"] = storage;
    }

    // ---- Write JSON ----
    std::ofstream out(filename);
    if (!out.is_open()) {
        std::string msg = "Failed to open output file: " + filename.string();
        capiocl::print_message(capiocl::CLI_LEVEL_ERROR, msg);
        ERR_EXIT(msg.c_str());
    }

    out << std::setw(4) << doc << std::endl;

    capiocl::print_message(capiocl::CLI_LEVEL_INFO,
                           "Configuration serialized to " + filename.string());
}
