#include <fstream>

#include "capiocl.hpp"
#include "capiocl/engine.h"
#include "capiocl/printer.h"
#include "capiocl/serializer.h"

void capiocl::serializer::Serializer::available_serializers::serialize_v1(
    const engine::Engine &engine, const std::filesystem::path &filename) {
    jsoncons::json doc;
    doc["name"] = engine.getWorkflowName();

    const auto files = engine._capio_cl_entries;

    std::unordered_map<std::string, std::vector<std::string>> app_inputs;
    std::unordered_map<std::string, std::vector<std::string>> app_outputs;

    std::vector<std::string> permanent;
    std::vector<std::string> exclude;
    std::vector<std::string> memory_storage;
    std::vector<std::string> fs_storage;

    jsoncons::json storage  = jsoncons::json::object();
    jsoncons::json io_graph = jsoncons::json::array();

    for (const auto &[path, entry] : files) {
        if (entry.permanent) {
            permanent.push_back(path);
        }
        if (entry.excluded) {
            exclude.push_back(path);
        }
        (entry.store_in_memory ? memory_storage : fs_storage).push_back(path);

        for (const auto &p : entry.producers) {
            app_outputs[p].push_back(path);
        }
        for (const auto &c : entry.consumers) {
            app_inputs[c].push_back(path);
        }
    }

    for (const auto &[app_name, outputs] : app_outputs) {
        jsoncons::json app       = jsoncons::json::object();
        jsoncons::json streaming = jsoncons::json::array();

        for (const auto &path : outputs) {
            const auto &entry = files.at(path);

            jsoncons::json streaming_item = jsoncons::json::object();
            std::string committed         = entry.commit_rule;
            const char *name_kind         = entry.is_file ? "name" : "dirname";
            streaming_item[name_kind]     = jsoncons::json::array({path}); // LCOV_EXCL_LINE

            if (entry.commit_on_close_count > 0) {
                if (entry.commit_rule == commitRules::ON_CLOSE) {
                    const auto close_count      = std::to_string(entry.commit_on_close_count);
                    streaming_item["committed"] = entry.commit_rule + ":" + close_count;
                } else {
                    const auto msg = "Commit rule is not ON_CLOSE but close count > 0";
                    printer::print(printer::CLI_LEVEL_WARNING, msg);
                    printer::print(printer::CLI_LEVEL_WARNING, "Setting commit rule = ON_CLOSE");
                    streaming_item["committed"] = std::string(commitRules::ON_CLOSE) + ":" +
                                                  std::to_string(entry.commit_on_close_count);
                }
            } else {
                streaming_item["committed"] = entry.commit_rule;
            }

            if (!entry.is_file) {
                streaming_item["n_files"] = entry.directory_children_count;
            }

            // Convert std::vector<std::filesystem::path> -> std::vector<std::string>
            std::vector<std::string> file_deps_str;
            file_deps_str.reserve(entry.file_dependencies.size());
            for (const auto &p : entry.file_dependencies) {
                file_deps_str.push_back(p.string());
            }
            streaming_item["file_deps"] = file_deps_str;

            streaming_item["mode"] = entry.fire_rule;

            streaming.push_back(streaming_item);
        }

        app["name"]          = app_name;
        app["input_stream"]  = app_inputs[app_name];
        app["output_stream"] = outputs;
        app["streaming"]     = streaming;

        io_graph.push_back(app);
    }

    // Ensure apps that only have inputs appear as well
    for (const auto &[app_name, inputs] : app_inputs) {
        bool contained = false;
        for (const auto &entry : io_graph.array_range()) {
            if (entry["name"].as<std::string>() == app_name) {
                contained = true;
                break;
            }
        }
        if (!contained) {
            jsoncons::json app   = jsoncons::json::object();
            app["name"]          = app_name;
            app["input_stream"]  = inputs;
            app["output_stream"] = jsoncons::json::array();
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
    if (!out.is_open()) {
        throw SerializerException("Failed to open output file: " + filename.string());
    }
    out << jsoncons::pretty_print(doc) << std::endl;

    printer::print(printer::CLI_LEVEL_INFO, "Configuration serialized to " + filename.string());
}