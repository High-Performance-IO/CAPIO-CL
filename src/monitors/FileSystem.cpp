#include <fstream>
#include <unistd.h>

#include "calf/StlLogger.h"
#include "capiocl.hpp"
#include "capiocl/monitor.h"

std::filesystem::path
capiocl::monitor::FileSystemMonitor::compute_capiocl_token_name(const std::filesystem::path &path,
                                                                 CAPIO_CL_COMMIT_TOKEN_TYPES type) {
    START_LOG(calf_current_tid(), "call()");
    std::string token_type;

    if (type == COMMIT) {
        token_type = ".commit";
    } else {
        token_type = ".home_node";
    }

    const auto abs          = std::filesystem::absolute(path);
    const auto new_filename = "." + abs.filename().string() + token_type;
    return abs.parent_path() / new_filename;
}

void capiocl::monitor::FileSystemMonitor::generate_home_node_token(
    const std::filesystem::path &path, const std::string &home_node) {
    START_LOG(calf_current_tid(), "call()");
    if (const auto token_name = compute_capiocl_token_name(path, HOME_NODE);
        !std::filesystem::exists(token_name)) {
        std::filesystem::create_directories(token_name.parent_path());
        std::ofstream file(token_name);
        file << home_node << std::endl;
        if (!file.good()) {
            LOG("failed to write home-node token=%s node=%s", token_name.string().c_str(),
                home_node.c_str());
        } else {
            LOG("created home-node token=%s node=%s", token_name.string().c_str(),
                home_node.c_str());
        }
        file.close();
    }
}

void capiocl::monitor::FileSystemMonitor::generate_commit_token(const std::filesystem::path &path) {
    START_LOG(calf_current_tid(), "call()");
    if (const auto token_name = compute_capiocl_token_name(path, COMMIT);
        !std::filesystem::exists(token_name)) {
        std::filesystem::create_directories(token_name.parent_path());
        std::ofstream file(token_name);
        if (!file.good()) {
            LOG("failed to create commit token=%s", token_name.string().c_str());
        } else {
            LOG("created commit token=%s", token_name.string().c_str());
        }
        file.close();
    }
}

capiocl::monitor::FileSystemMonitor::FileSystemMonitor() {
    START_LOG(calf_current_tid(), "call()");
    gethostname(_hostname, sizeof(_hostname));
    _hostname[sizeof(_hostname) - 1] = '\0';
    LOG("filesystem monitor initialized hostname=%s", _hostname);
}

void capiocl::monitor::FileSystemMonitor::setCommitted(const std::filesystem::path &path) const {
    START_LOG(calf_current_tid(), "call()");
    generate_commit_token(path);
}

bool capiocl::monitor::FileSystemMonitor::isCommitted(const std::filesystem::path &path) const {
    START_LOG(calf_current_tid(), "call()");
    return std::filesystem::exists(compute_capiocl_token_name(path));
}

void capiocl::monitor::FileSystemMonitor::setHomeNode(const std::filesystem::path &path) const {
    START_LOG(calf_current_tid(), "call()");
    generate_home_node_token(path, _hostname);
}

std::string
capiocl::monitor::FileSystemMonitor::getHomeNode(const std::filesystem::path &path) const {
    START_LOG(calf_current_tid(), "call()");
    auto home_node_token = compute_capiocl_token_name(path, HOME_NODE);

    std::lock_guard lg(home_node_lock);

    if (const auto it = _home_nodes.find(path); it != _home_nodes.end()) {
        LOG("home-node cache hit path=%s node=%s", path.string().c_str(), it->second.c_str());
        return it->second;
    }

    std::string home_node;
    if (!std::filesystem::exists(home_node_token)) {
        home_node = NO_HOME_NODE;
    } else {
        std::ifstream file(home_node_token);
        file >> home_node;
    }

    auto [entry, _] = _home_nodes.emplace(path, std::move(home_node));
    LOG("resolved home node path=%s node=%s", path.string().c_str(), entry->second.c_str());
    return entry->second;
}
