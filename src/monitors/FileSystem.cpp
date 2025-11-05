#include <fstream>
#include <iostream>

#include "capiocl.hpp"

std::filesystem::path
capiocl::FileSystemMonitor::compute_commit_token_name(const std::filesystem::path &path) {
    const auto abs = std::filesystem::absolute(path);
    auto token     = abs.parent_path() / ("." + abs.filename().string() + ".capiocl");
    return token;
}

void capiocl::FileSystemMonitor::generate_commit_token(const std::filesystem::path &path) {
    if (const auto token_name = compute_commit_token_name(path);
        !std::filesystem::exists(token_name)) {
        std::filesystem::create_directories(token_name.parent_path());
        std::ofstream file(token_name);
        file.close();
    }
}

void capiocl::FileSystemMonitor::setCommitted(const std::filesystem::path &path) const {
    generate_commit_token(path);
}

bool capiocl::FileSystemMonitor::isCommitted(const std::filesystem::path &path) const {
    return std::filesystem::exists(compute_commit_token_name(path));
}
