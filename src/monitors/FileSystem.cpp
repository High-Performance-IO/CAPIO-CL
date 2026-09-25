#include <cerrno>
#include <chrono>
#include <fcntl.h>
#include <fstream>
#include <limits>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <utility>

#include "calf/StlLogger.h"
#include "capiocl.hpp"
#include "capiocl/monitor.h"

[[noreturn]] void monitor_error(const std::string &operation, const std::filesystem::path &path,
                                const int error = errno) {
    throw capiocl::monitor::MonitorException(operation + " " + path.string() + ": " +
                                             strerror(error));
}

struct CloseMetadataPaths final {
    std::filesystem::path counter;
    std::filesystem::path lock;
};

CloseMetadataPaths close_metadata_paths(const std::filesystem::path &path,
                                        const std::filesystem::path &metadata_root) {
    if (metadata_root.empty()) {
        throw capiocl::monitor::MonitorException(
            "Counted ON_CLOSE requires capiocl.monitor.filesystem.metadata_dir to name a trusted, "
            "unique workflow metadata directory");
    }
    const auto root = std::filesystem::absolute(metadata_root).lexically_normal() / "capiocl";
    const auto normalized = std::filesystem::absolute(path).lexically_normal().generic_string();
    static constexpr char digits[] = "0123456789abcdef";
    std::string encoded;
    encoded.reserve(normalized.size() * 2);
    for (const unsigned char character : normalized) {
        encoded.push_back(digits[character >> 4]);
        encoded.push_back(digits[character & 0x0f]);
    }

    auto make_directory = [&](const char *subtree) {
        auto directory = root / subtree;
        for (size_t offset = 0; offset < encoded.size(); offset += 64) {
            directory /= encoded.substr(offset, 64);
        }
        std::error_code error;
        std::filesystem::create_directories(directory, error);
        const auto status = std::filesystem::symlink_status(directory, error);
        if (error || std::filesystem::is_symlink(status) ||
            !std::filesystem::is_directory(status)) {
            throw capiocl::monitor::MonitorException("Unsafe CAPIO-CL metadata directory " +
                                                     directory.string());
        }
        return directory;
    };
    return {make_directory("close-counts") / "count", make_directory("close-locks") / "lock"};
}

long read_counter(const std::filesystem::path &path) {
    std::error_code error;
    const auto status = std::filesystem::symlink_status(path, error);
    if (error == std::errc::no_such_file_or_directory) {
        return 0;
    }
    if (error || !std::filesystem::exists(status) || std::filesystem::is_symlink(status) ||
        !std::filesystem::is_regular_file(status)) {
        throw capiocl::monitor::MonitorException("Unsafe close counter " + path.string());
    }
    std::ifstream file(path);
    long value = 0;
    if (!(file >> value) || value < 0) {
        throw capiocl::monitor::MonitorException("Malformed close counter " + path.string());
    }
    file >> std::ws;
    if (!file.eof()) {
        throw capiocl::monitor::MonitorException("Malformed close counter " + path.string());
    }
    return value;
}

// ponytail: atomic rename avoids partial counters; a machine crash can still lose the latest close.
void persist_counter(const std::filesystem::path &counter, const long value) {
    auto temporary = counter;
    temporary += ".tmp";
    std::error_code error;
    if (std::filesystem::is_symlink(std::filesystem::symlink_status(temporary, error))) {
        throw capiocl::monitor::MonitorException("Unsafe temporary close counter " +
                                                 temporary.string());
    }
    try {
        {
            std::ofstream file(temporary, std::ios::trunc);
            file << value << '\n';
            file.close();
            if (!file.good()) {
                throw capiocl::monitor::MonitorException("Unable to write close counter " +
                                                         temporary.string());
            }
        }
        const auto status = std::filesystem::symlink_status(counter, error);
        if (!error && std::filesystem::is_symlink(status)) {
            throw capiocl::monitor::MonitorException("Unsafe close counter " + counter.string());
        }
        std::filesystem::rename(temporary, counter, error);
        if (error) {
            throw capiocl::monitor::MonitorException("Unable to replace close counter " +
                                                     counter.string() + ": " + error.message());
        }
    } catch (...) {
        std::filesystem::remove(temporary, error);
        throw;
    }
}

class FileLock final {
    std::filesystem::path path;
    int fd = -1;

  public:
    explicit FileLock(std::filesystem::path lock_path) : path(std::move(lock_path)) {
        while (true) {
            fd = open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
            if (fd != -1) {
                return;
            }
            const int error = errno;
            if (error != EEXIST) {
                monitor_error("Unable to acquire close counter lock", path, error);
            }
            struct stat status{};
            if (lstat(path.c_str(), &status) == 0) {
                if (!S_ISREG(status.st_mode)) {
                    throw capiocl::monitor::MonitorException("Unsafe close counter lock " +
                                                             path.string());
                }
            } else if (errno != ENOENT) {
                monitor_error("Unable to inspect close counter lock", path);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    ~FileLock() {
        if (fd == -1) {
            return;
        }
        struct stat owned{}, current{};
        const bool still_owned = fstat(fd, &owned) == 0 && lstat(path.c_str(), &current) == 0 &&
                                 owned.st_dev == current.st_dev && owned.st_ino == current.st_ino;
        close(fd);
        if (still_owned) {
            unlink(path.c_str());
        }
    }

    FileLock(const FileLock &)            = delete;
    FileLock &operator=(const FileLock &) = delete;
    FileLock(FileLock &&)                 = delete;
    FileLock &operator=(FileLock &&)      = delete;
};

std::filesystem::path
capiocl::monitor::FileSystemMonitor::compute_capiocl_token_name(const std::filesystem::path &path,
                                                                CAPIO_CL_COMMIT_TOKEN_TYPES type) {
    START_LOG(calf_current_tid(), "call()");
    std::string token_type;

    if (type == COMMIT) {
        token_type = ".commit";
    } else if (type == HOME_NODE) {
        token_type = ".home_node";
    }

    const auto abs          = std::filesystem::absolute(path).lexically_normal();
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
    const auto token_name = compute_capiocl_token_name(path, COMMIT);
    std::filesystem::create_directories(token_name.parent_path());
    std::error_code error;
    const auto status = std::filesystem::symlink_status(token_name, error);
    if (!error &&
        (std::filesystem::is_symlink(status) || !std::filesystem::is_regular_file(status))) {
        throw MonitorException("Unsafe commit token " + token_name.string());
    }
    std::ofstream file(token_name, std::ios::app);
    if (!file.good()) {
        throw MonitorException("Unable to create commit token " + token_name.string());
    }
    LOG("created commit token=%s", token_name.string().c_str());
}

capiocl::monitor::FileSystemMonitor::FileSystemMonitor()
    : FileSystemMonitor(configuration::CapioClConfiguration{}) {}

capiocl::monitor::FileSystemMonitor::FileSystemMonitor(
    const configuration::CapioClConfiguration &config) {
    START_LOG(calf_current_tid(), "call()");
    std::string configured_metadata_root;
    config.getParameter("capiocl.monitor.filesystem.metadata_dir", &configured_metadata_root,
                        configuration::defaults::DEFAULT_MONITOR_FS_METADATA_DIR.v);
    metadata_root = configured_metadata_root;
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

std::optional<bool>
capiocl::monitor::FileSystemMonitor::increaseCloseCount(const std::filesystem::path &path,
                                                        const long threshold) const {
    START_LOG(calf_current_tid(), "call()");
    if (threshold <= 1) {
        throw std::invalid_argument("Persistent ON_CLOSE threshold must be greater than one");
    }
    const auto metadata = close_metadata_paths(path, metadata_root);
    FileLock lock(metadata.lock);
    long value = read_counter(metadata.counter);

    if (value == std::numeric_limits<long>::max()) {
        throw MonitorException("Close counter overflow for " + metadata.counter.string());
    }
    ++value;
    if (value >= threshold) {
        generate_commit_token(path);
    }
    persist_counter(metadata.counter, value);
    return value >= threshold;
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
