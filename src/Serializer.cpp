#include <algorithm>
#include <filesystem>
#include <fstream>
#include <unordered_set>
#include <jsoncons/json.hpp>

#include "calf/StdOutLogger.h"
#include "calf/StlLogger.h"
#include "capiocl.hpp"
#include "capiocl/engine.h"
#include "capiocl/serializer.h"

void capiocl::serializer::Serializer::dump(const engine::Engine &engine,
                                           const std::filesystem::path &filename,
                                           const bool compress, const std::string &version) {
    START_LOG(calf_current_tid(), "call()");
    UPDATE_CALF_WORKFLOW_NAME(engine.getWorkflowName());
    if (version == CAPIO_CL_VERSION::V1) {
        CALF_PRINT_COLOR(CALF_CLI_LEVEL_INFO, "Serializing engine with V1 specification");
        available_serializers::serialize_v1(engine, filename, compress);
    } else if (version == CAPIO_CL_VERSION::V1_1) {
        CALF_PRINT_COLOR(CALF_CLI_LEVEL_INFO, "Serializing engine with V1.1 specification");
        available_serializers::serialize_v1_1(engine, filename, compress);
    } else {
        LOG("serializer unavailable version=%s workflow=%s output=%s", version.c_str(),
            engine.getWorkflowName().c_str(), filename.string().c_str());
        const auto message = "No serializer available for CAPIO-CL version: " + version;
        throw SerializerException(message);
    }
}

capiocl::serializer::SerializerException::SerializerException(const std::string &msg)
    : message(msg) {
    START_LOG(calf_current_tid(), "call()");
    UPDATE_CALF_WORKFLOW_NAME("");
    CALF_PRINT_COLOR(CALF_CLI_LEVEL_ERROR, "%s", msg.c_str());
}

void capiocl::serializer::Serializer::sortPathsByDecreasingLength(std::vector<std::string> &paths) {
    std::sort(paths.begin(), paths.end(), [](const std::string &a, const std::string &b) {
        if (a.length() != b.length()) {
            return a.length() > b.length();
        }
        return a > b;
    });
}

std::vector<std::pair<std::string, std::string>>
capiocl::serializer::Serializer::compressedPaths(const engine::Engine &engine) {
    std::unordered_map<std::string, std::string> paths;
    std::unordered_map<std::string, std::unordered_set<std::string>> candidates;
    std::vector<std::string> directories;
    std::unordered_set<std::string> known_directories;

    const auto for_each_parent = [](const std::string &path, const auto &callback) {
        for (auto parent = std::filesystem::path(path).parent_path();;) {
            callback(parent.string());
            if (parent.empty() || parent == parent.root_path()) {
                break;
            }
            parent = parent.parent_path();
        }
    };

    for (const auto &[path, entry] : engine._capio_cl_entries) {
        paths.emplace(path, path);
        if (!entry.is_file || path.find_first_of("*?[") != std::string::npos) {
            continue;
        }

        // Index each file under its ancestors once. Compression then examines
        // only descendants of the current directory instead of every path.
        for_each_parent(path, [&](const std::string &parent) {
            candidates[parent].insert(path);
            if (known_directories.insert(parent).second) {
                directories.push_back(parent);
            }
        });
    }

    sortPathsByDecreasingLength(directories);

    for (const auto &directory : directories) {
        const auto wildcard = (std::filesystem::path(directory) / "*").string();
        if (paths.find(wildcard) != paths.end()) {
            continue;
        }

        std::vector<std::vector<std::string>> groups;
        for (const auto &output : candidates[directory]) {
            const auto path = paths.find(output);
            if (path == paths.end()) {
                continue;
            }
            const auto &source = path->second;
            if (!engine._capio_cl_entries.at(source).is_file ||
                (output == source && output.find_first_of("*?[") != std::string::npos)) {
                continue;
            }

            auto group = std::find_if(groups.begin(), groups.end(), [&](const auto &candidate) {
                return engine._capio_cl_entries.at(paths.at(candidate.front())) ==
                       engine._capio_cl_entries.at(source);
            });
            (group == groups.end() ? groups.emplace_back() : *group).push_back(output);
        }
        for (auto &group : groups) {
            std::sort(group.begin(), group.end());
        }

        const auto largest =
            std::max_element(groups.begin(), groups.end(), [](const auto &left, const auto &right) {
                return left.size() != right.size() ? left.size() < right.size()
                                                   : left.front() > right.front();
            });
        if (largest == groups.end() || largest->size() < 2) {
            continue;
        }

        const auto source = paths.at(largest->front());
        for (const auto &path : *largest) {
            paths.erase(path);
            for_each_parent(path,
                            [&](const std::string &parent) { candidates[parent].erase(path); });
        }
        paths.emplace(wildcard, source);

        // Parent directories must see the replacement wildcard, but this
        // directory has already selected its one compression group.
        for_each_parent(wildcard, [&](const std::string &parent) {
            if (parent != directory) {
                candidates[parent].insert(wildcard);
            }
        });
        CALF_PRINT_COLOR(CALF_CLI_LEVEL_WARNING, "Compressing entries to %s", wildcard.c_str());
    }

    return {paths.begin(), paths.end()};
}
