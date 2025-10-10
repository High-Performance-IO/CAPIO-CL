#include "capiocl.hpp"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

void capiocl::Serializer::dump(const Engine &engine, const std::string &workflow_name,
                               const std::filesystem::path &filename) {

    nlohmann::json doc;
    doc["name"] = workflow_name;

    const auto *files = engine.getLocations();

    std::unordered_map<std::string, std::vector<std::string>> app_inputs;
    std::unordered_map<std::string, std::vector<std::string>> app_outputs;

    std::vector<std::string> permanent;
    std::vector<std::string> exclude;
    std::vector<std::string> memory_storage;
    std::vector<std::string> fs_storage;

    nlohmann::json storage;
    nlohmann::json io_graph = nlohmann::json::array();

    for (const auto &[path, data] : *files) {
        const auto &[producers, consumers, commit_rule, fire_rule, permanent_flag, excluded_flag,
                     is_file, n_close, n_dir_files, file_deps, store_in_memory] = data;

        if (permanent_flag) {
            permanent.push_back(path);
        }
        if (excluded_flag) {
            exclude.push_back(path);
        }

        store_in_memory ? memory_storage.push_back(path) : fs_storage.push_back(path);

        for (const auto &p : producers) {
            app_outputs[p].push_back(path);
        }
        for (const auto &c : consumers) {
            app_inputs[c].push_back(path);
        }
    }

    for (const auto &[app_name, outputs] : app_outputs) {
        nlohmann::json app;
        nlohmann::json streaming = nlohmann::json::array();

        for (const auto &path : outputs) {
            const auto &data = files->at(path);
            const auto &[producers, consumers, commit_rule, fire_rule, permanent_flag,
                         excluded_flag, is_file, n_close, n_dir_files, file_deps, store_in_fs] =
                data;

            nlohmann::json streaming_item;
            std::string committed     = commit_rule;
            const auto name_kind      = is_file ? "name" : "dirname";
            streaming_item[name_kind] = {path};

            // Commit rule
            if (commit_rule == commit_rules::ON_CLOSE && n_close > 0) {
                committed += ":" + std::to_string(n_close);
            }
            if (commit_rule == commit_rules::N_FILES && n_dir_files > 0) {
                committed += ":" + std::to_string(n_dir_files);
            }

            streaming_item["file_deps"] = file_deps;
            streaming_item["committed"] = committed;
            streaming_item["mode"]      = fire_rule;
            streaming_item["n_files"]   = n_dir_files;

            streaming.push_back(streaming_item);
        }

        app["name"]          = app_name;
        app["input_stream"]  = app_inputs[app_name];
        app["output_stream"] = outputs;
        app["streaming"]     = streaming;

        io_graph.push_back(app);
    }

    /// Check for app names that have a output_stream empty
    for (const auto &[app_name, inputs] : app_inputs) {
        bool contained = false;
        for (const auto &entry : io_graph) {
            contained = entry["name"] == app_name;
            if (contained) {
                break;
            }
        }
        if (!contained) {
            nlohmann::json app;
            app["name"]          = app_name;
            app["input_stream"]  = inputs;
            app["output_stream"] = nlohmann::json::array();
            io_graph.push_back(app);
        }
    }

    doc["IO_Graph"]   = io_graph;
    doc["permanent"]  = permanent;
    doc["exclude"]    = exclude;
    storage["memory"] = memory_storage;
    storage["fs"]     = fs_storage;
    doc["storage"]    = storage;

    std::ofstream out(filename);
    out << std::setw(2) << doc << std::endl;

    print_message(CLI_LEVEL_INFO, "Configuration serialized to " + filename.string());
}
