#include <fstream>
#include <unistd.h>

#include "capiocl.hpp"
#include "capiocl/monitor.h"

std::filesystem::path
capiocl::monitor::FileSystemMonitor::compute_capiocl_token_name(const std::filesystem::path &path,
                                                                CAPIO_CL_COMMIT_TOKEN_TYPES type) {
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
    if (const auto token_name = compute_capiocl_token_name(path, HOME_NODE);
        !std::filesystem::exists(token_name)) {
        std::filesystem::create_directories(token_name.parent_path());
        std::ofstream file(token_name);
        file << home_node << std::endl;
        file.close();
    }
}

void capiocl::monitor::FileSystemMonitor::generate_commit_token(const std::filesystem::path &path) {
    if (const auto token_name = compute_capiocl_token_name(path, COMMIT);
        !std::filesystem::exists(token_name)) {
        std::filesystem::create_directories(token_name.parent_path());
        std::ofstream file(token_name);
        file.close();
    }
}

capiocl::monitor::FileSystemMonitor::FileSystemMonitor() { gethostname(_hostname, HOST_NAME_MAX); }

void capiocl::monitor::FileSystemMonitor::setCommitted(const std::filesystem::path &path) const {
    generate_commit_token(path);
}

bool capiocl::monitor::FileSystemMonitor::isCommitted(const std::filesystem::path &path) const {
    return std::filesystem::exists(compute_capiocl_token_name(path));
}

void capiocl::monitor::FileSystemMonitor::setHomeNode(const std::filesystem::path &path) const {
    generate_home_node_token(path, _hostname);
}

const std::string &
capiocl::monitor::FileSystemMonitor::getHomeNode(const std::filesystem::path &path) const {

    auto home_node_token = compute_capiocl_token_name(path, HOME_NODE);

    std::lock_guard lg(home_node_lock);

    if (const auto it = _home_nodes.find(path); it != _home_nodes.end()) {
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
    return entry->second;
}