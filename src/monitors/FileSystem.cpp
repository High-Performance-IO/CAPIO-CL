#include <fstream>
#include <iostream>

#include "capiocl.hpp"
#include "include/monitor.h"

std::filesystem::path
capiocl::monitor::FileSystemMonitor::compute_commit_token_name(const std::filesystem::path &path) {
    const auto abs = std::filesystem::absolute(path);
    auto token     = abs.parent_path() / ("." + abs.filename().string() + ".capiocl");
    return token;
}

void capiocl::monitor::FileSystemMonitor::generate_commit_token(const std::filesystem::path &path) {
    if (const auto token_name = compute_commit_token_name(path);
        !std::filesystem::exists(token_name)) {
        std::filesystem::create_directories(token_name.parent_path());
        std::ofstream file(token_name);
        file.close();
    }
}

void capiocl::monitor::FileSystemMonitor::setCommitted(const std::filesystem::path &path) const {
    generate_commit_token(path);
}

bool capiocl::monitor::FileSystemMonitor::isCommitted(const std::filesystem::path &path) const {
    return std::filesystem::exists(compute_commit_token_name(path));
}

void capiocl::monitor::FileSystemMonitor::setHomeNode(const std::filesystem::path &path) const {
    return;
}

const std::string &
capiocl::monitor::FileSystemMonitor::getHomeNode(const std::filesystem::path &path) const {
    return this->ho_home_node;
}