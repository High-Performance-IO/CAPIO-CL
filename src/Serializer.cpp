#include <algorithm>
#include <filesystem>
#include <fstream>
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
    std::vector<std::string> directories;

    for (const auto &[path, entry] : engine._capio_cl_entries) {
        paths.emplace(path, path);
        if (!entry.is_file || path.find_first_of("*?[") != std::string::npos) {
            continue;
        }

        for (auto parent = std::filesystem::path(path).parent_path();;) {
            if (const auto value = parent.string();
                std::find(directories.begin(), directories.end(), value) == directories.end()) {
                directories.push_back(value);
            }
            if (parent.empty() || parent == parent.root_path()) {
                break;
            }
            parent = parent.parent_path();
        }
    }

    sortPathsByDecreasingLength(directories);

    for (const auto &directory : directories) {
        const auto wildcard = (std::filesystem::path(directory) / "*").string();
        if (paths.find(wildcard) != paths.end()) {
            continue;
        }

        std::vector<std::vector<std::string>> groups;
        for (const auto &[output, source] : paths) {
            if (!engine._capio_cl_entries.at(source).is_file ||
                (output == source && output.find_first_of("*?[") != std::string::npos)) {
                continue;
            }
            const std::filesystem::path output_path(output);
            const auto relative = output_path.lexically_relative(directory);
            if ((!directory.empty() &&
                 (relative.empty() || *relative.begin() == std::filesystem::path(".."))) ||
                (directory.empty() && output_path.is_absolute())) {
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
        }
        paths.emplace(wildcard, source);
        CALF_PRINT_COLOR(CALF_CLI_LEVEL_WARNING, "Compressing entries to %s", wildcard.c_str());
    }

    return {paths.begin(), paths.end()};
}
