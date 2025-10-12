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

    for (const auto &[path, entry] : *files) {

        if (entry.permanent) {
            permanent.push_back(path);
        }
        if (entry.excluded) {
            exclude.push_back(path);
        }

        entry.store_in_memory ? memory_storage.push_back(path) : fs_storage.push_back(path);

        for (const auto &p : entry.producers) {
            app_outputs[p].push_back(path);
        }
        for (const auto &c : entry.consumers) {
            app_inputs[c].push_back(path);
        }
    }

    for (const auto &[app_name, outputs] : app_outputs) {
        nlohmann::json app;
        nlohmann::json streaming = nlohmann::json::array();

        for (const auto &path : outputs) {
            const auto &entry = files->at(path);

            nlohmann::json streaming_item;
            std::string committed     = entry.commit_rule;
            const auto name_kind      = entry.is_file ? "name" : "dirname";
            streaming_item[name_kind] = {path};

            // Commit rule
            if (entry.commit_rule == commit_rules::ON_CLOSE && entry.commit_on_close_count > 0) {
                committed += ":" + std::to_string(entry.commit_on_close_count);
            }

            if (!entry.is_file) {
                streaming_item["n_files"] = entry.directory_commit_file_count;
            }

            streaming_item["file_deps"] = entry.file_dependencies;
            streaming_item["committed"] = committed;
            streaming_item["mode"]      = entry.fire_rule;

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
