#include "capiocl.hpp"
#include <algorithm>
#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <shlwapi.h>
#define fnmatch(pattern, string, flags) (PathMatchSpecW(string, pattern) ? 0 : 1)
#else
#include <fnmatch.h>
#endif

void capiocl::Engine::print() const {
    // First message
    print_message(CLI_LEVEL_JSON, "");
    print_message(CLI_LEVEL_JSON, "Composition of expected CAPIO FS: ");

    // Table header lines
    print_message(CLI_LEVEL_JSON,
                  "|============================================================================"
                  "==========================================================|");

    print_message(CLI_LEVEL_JSON, "|" + std::string(134, ' ') + "|");

    {
        std::ostringstream oss;
        oss << "|     Parsed configuration file for workflow: \033[1;36m" << node_name
            << std::setw(94 - node_name.length()) << "\033[0m |";
        print_message(CLI_LEVEL_JSON, oss.str());
    }

    print_message(CLI_LEVEL_JSON, "|" + std::string(134, ' ') + "|");

    print_message(CLI_LEVEL_JSON,
                  "|     File color legend:     \033[48;5;034m  \033[0m File stored in memory" +
                      std::string(82, ' ') + "|");

    print_message(
        CLI_LEVEL_JSON,
        "|                            \033[48;5;172m  \033[0m File stored on file system" +
            std::string(77, ' ') + "|");

    print_message(CLI_LEVEL_JSON,
                  "|============================================================================"
                  "==========================================================|");

    print_message(CLI_LEVEL_JSON,
                  "|======|===================|===================|====================|========"
                  "============|============|===========|=========|==========|");

    print_message(CLI_LEVEL_JSON,
                  "| Kind | Filename          | Producer step     | Consumer step      |  "
                  "Commit Rule       |  Fire Rule | Permanent | Exclude | n_files  |");

    print_message(CLI_LEVEL_JSON,
                  "|======|===================|===================|====================|========"
                  "============|============|===========|=========|==========|");

    // Iterate over _locations
    for (auto &itm : _locations) {
        std::string color_preamble = std::get<10>(itm.second) ? "\033[38;5;034m" : "\033[38;5;172m";
        std::string color_post     = "\033[0m";

        std::string name_trunc = truncateLastN(itm.first.string(), 12);
        auto kind              = std::get<6>(itm.second) ? "F" : "D";

        std::ostringstream base_line;
        base_line << "|   " << color_preamble << kind << color_post << "  | " << color_preamble
                  << name_trunc << color_post << std::setfill(' ')
                  << std::setw(20 - name_trunc.length()) << "| ";

        auto producers = std::get<0>(itm.second);
        auto consumers = std::get<1>(itm.second);
        auto rowCount  = std::max(producers.size(), consumers.size());

        std::string n_files = std::to_string(std::get<8>(itm.second));
        if (std::get<8>(itm.second) < 1) {
            n_files = "N.A.";
        }

        for (std::size_t i = 0; i <= rowCount; i++) {
            std::ostringstream line;

            if (i == 0) {
                line << base_line.str();
            } else {
                line << "|      |                   | ";
            }

            if (i < producers.size()) {
                auto prod1 = truncateLastN(producers.at(i), 12);
                line << prod1 << std::setfill(' ') << std::setw(20 - prod1.length()) << " | ";
            } else {
                line << std::setfill(' ') << std::setw(20) << " | ";
            }

            if (i < consumers.size()) {
                auto cons1 = truncateLastN(consumers.at(i), 12);
                line << " " << cons1 << std::setfill(' ') << std::setw(20 - cons1.length())
                     << " | ";
            } else {
                line << std::setfill(' ') << std::setw(21) << " | ";
            }

            if (i == 0) {
                std::string commit_rule = std::get<2>(itm.second),
                            fire_rule   = std::get<3>(itm.second);
                bool exclude = std::get<5>(itm.second), permanent = std::get<4>(itm.second);

                line << " " << commit_rule << std::setfill(' ')
                     << std::setw(20 - commit_rule.length()) << " | " << fire_rule
                     << std::setfill(' ') << std::setw(13 - fire_rule.length()) << " | "
                     << "    " << (permanent ? "YES" : "NO ") << "   |   "
                     << (exclude ? "YES" : "NO ") << "   | " << n_files
                     << std::setw(10 - n_files.length()) << " |";
            } else {
                line << std::setfill(' ') << std::setw(20) << "|" << std::setfill(' ')
                     << std::setw(13) << "|" << std::setfill(' ') << std::setw(12) << "|"
                     << std::setfill(' ') << std::setw(10) << "|" << std::setw(10) << "|";
            }

            print_message(CLI_LEVEL_JSON, line.str());
        }

        print_message(CLI_LEVEL_JSON,
                      "*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
                      "~~~~~~~~~~~~~~~~~~"
                      "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*");
    }

    print_message(CLI_LEVEL_JSON, "");
}

bool capiocl::Engine::contains(const std::filesystem::path &file) {
    START_LOG(gettid(), "call(file=%s)", file.c_str());
    for (auto &[fst, snd] : _locations) {
        if (fnmatch(file.c_str(), fst.c_str(), FNM_PATHNAME) == 0) {
            return true;
        }
    }
    return false;
}
size_t capiocl::Engine::size() const { return this->_locations.size(); }

void capiocl::Engine::add(std::filesystem::path &path, std::vector<std::string> &producers,
                          std::vector<std::string> &consumers, const std::string &commit_rule,
                          const std::string &fire_rule, bool permanent, bool exclude,
                          const std::vector<std::filesystem::path> &dependencies) {
    START_LOG(gettid(), "call(path=%s, commit=%s, fire=%s, permanent=%s, exclude=%s)", path.c_str(),
              commit_rule.c_str(), fire_rule.c_str(), permanent ? "YES" : "NO",
              exclude ? "YES" : "NO");

    _locations.emplace(path, std::make_tuple(producers, consumers, commit_rule, fire_rule,
                                             permanent, exclude, true, 0, 0, dependencies, false));
}

void capiocl::Engine::newFile(const std::filesystem::path &path) {
    START_LOG(gettid(), "call(path=%s)", path.c_str());
    if (_locations.find(path) == _locations.end()) {
        std::string commit = COMMITTED_ON_TERMINATION;
        std::string fire   = MODE_UPDATE;

        /*
         * Inherit commit and fire rules from LPM (Longest Prefix Match) directory
         * matchSize is used to compute LPM
         */
        std::filesystem::path matchKey;
        size_t matchSize = 0;
        for (const auto &[filename, data] : _locations) {
            if (fnmatch(filename.c_str(), path.c_str(), FNM_PATHNAME) == 0 &&
                filename.string().length() > matchSize) {
                LOG("Found match with %s", filename.c_str());
                matchSize = filename.string().length();
                matchKey  = filename;
            }
        }

        if (matchSize > 0) {
            LOG("Adding file %s to _locations with commit=%s, and fire=%s", path.c_str(),
                commit.c_str(), fire.c_str());
            const auto data                              = _locations.at(matchKey);
            std::vector<std::string> prod                = std::get<0>(data);
            std::vector<std::string> cons                = std::get<1>(data);
            commit                                       = std::get<2>(data);
            fire                                         = std::get<3>(data);
            bool is_permanent                            = std::get<4>(data);
            bool is_excluded                             = std::get<5>(data);
            bool is_file                                 = std::get<6>(data);
            long committed_on_close_count                = std::get<7>(data);
            long expected_directory_file_count           = std::get<8>(data);
            std::vector<std::filesystem::path> file_deps = std::get<9>(data);
            bool store_in_fs                             = std::get<10>(data);
            _locations.emplace(path, std::make_tuple(prod, cons, commit, fire, is_permanent,
                                                     is_excluded, is_file, committed_on_close_count,
                                                     expected_directory_file_count, file_deps,
                                                     store_in_fs));

        } else {
            _locations.emplace(
                path, std::make_tuple(std::vector<std::string>(), std::vector<std::string>(),
                                      COMMITTED_ON_TERMINATION, MODE_UPDATE, false, false, true, 0,
                                      0, std::vector<std::filesystem::path>(), false));
        }
    }
}

long capiocl::Engine::getDirectoryFileCount(const std::filesystem::path &path) {
    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        return std::get<8>(itm->second);
    }
    this->newFile(path);

    return getDirectoryFileCount(path);
}

void capiocl::Engine::addProducer(const std::filesystem::path &path, std::string &producer) {
    START_LOG(gettid(), "call(path=%s, producer=%s)", path.c_str(), producer.c_str());
    producer.erase(remove_if(producer.begin(), producer.end(), isspace), producer.end());
    newFile(path);
    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        std::get<0>(itm->second).emplace_back(producer);
    }
}

void capiocl::Engine::addConsumer(const std::filesystem::path &path, std::string &consumer) {
    START_LOG(gettid(), "call(path=%s, consumer=%s)", path.c_str(), consumer.c_str());
    consumer.erase(remove_if(consumer.begin(), consumer.end(), isspace), consumer.end());
    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        std::get<1>(itm->second).emplace_back(consumer);
    }
}
void capiocl::Engine::addFileDependency(const std::filesystem::path &path,
                                        std::string &file_dependency) {
    START_LOG(gettid(), "call(path=%s, consumer=%s)", path.c_str(), consumer.c_str());
    file_dependency.erase(remove_if(file_dependency.begin(), file_dependency.end(), isspace),
                          file_dependency.end());
    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        std::get<9>(itm->second).emplace_back(file_dependency);
    }
}

void capiocl::Engine::setCommitRule(const std::filesystem::path &path,
                                    const std::string &commit_rule) {
    START_LOG(gettid(), "call(path=%s, commit_rule=%s)", path.c_str(), commit_rule.c_str());
    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        std::get<2>(itm->second) = commit_rule;
    }
}

std::string capiocl::Engine::getCommitRule(const std::filesystem::path &path) {
    START_LOG(gettid(), "call(path=%s)", path.c_str());
    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        LOG("Commit rule: %s", std::get<2>(_locations.at(path)).c_str());
        return std::get<2>(itm->second);
    }

    LOG("No entry found on map. checking globs. Creating new file from globs for cache purpose");
    this->newFile(path);
    LOG("Returning commit rule for file %s (update)", path.c_str());
    return getCommitRule(path);
}

std::string capiocl::Engine::getFireRule(const std::filesystem::path &path) {
    START_LOG(gettid(), "call(path=%s)", path.c_str());
    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        LOG("Fire rule: %s", std::get<3>(_locations.at(path)).c_str());
        return std::get<3>(itm->second);
    }

    LOG("No entry found on map. checking globs. Creating new file from globs for cache purpose");
    this->newFile(path);
    LOG("Returning Fire rule for file %s (update)", path.c_str());
    return getFireRule(path);
}

void capiocl::Engine::setFireRule(const std::filesystem::path &path, const std::string &fire_rule) {
    START_LOG(gettid(), "call(path=%s, fire_rule=%s)", path.c_str(), fire_rule.c_str());
    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        std::get<3>(itm->second) = fire_rule;
    }
}

bool capiocl::Engine::isFirable(const std::filesystem::path &path) {
    START_LOG(gettid(), "call(path=%s)", path.c_str());
    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        LOG("Fire rule for file %s is %s", path.c_str(), std::get<3>(itm->second).c_str());
        return std::get<3>(itm->second) == MODE_NO_UPDATE;
    }

    LOG("No entry found on map. checking globs. Creating new file from globs for cache purpose");
    this->newFile((path));
    LOG("Fire rule for file %s is  %s", path.c_str(), std::get<3>(_locations.at((path))).c_str());
    return std::get<3>(_locations.at((path))) == MODE_NO_UPDATE;
}

void capiocl::Engine::setPermanent(const std::filesystem::path &path, bool value) {
    START_LOG(gettid(), "call(path=%s, value=%s)", path.c_str(), value ? "true" : "false");
    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        std::get<4>(itm->second) = value;
    }
}

bool capiocl::Engine::isPermanent(const std::filesystem::path &path) {
    START_LOG(gettid(), "call(path=%s, value=%s)", path.c_str(), value ? "true" : "false");
    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        return std::get<4>(itm->second);
    }

    this->newFile(path);
    return isPermanent(path);
}

void capiocl::Engine::setExclude(const std::filesystem::path &path, const bool value) {
    START_LOG(gettid(), "call(path=%s, value=%s)", path.c_str(), value ? "true" : "false");
    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        std::get<5>(itm->second) = value;
    }
}

void capiocl::Engine::setDirectory(const std::filesystem::path &path) {
    START_LOG(gettid(), "call(path=%s)", path.c_str());
    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        std::get<6>(itm->second) = false;
    }
}

void capiocl::Engine::setFile(const std::filesystem::path &path) {
    START_LOG(gettid(), "call(path=%s)", path.c_str());
    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        std::get<6>(itm->second) = true;
    }
}

bool capiocl::Engine::isFile(const std::filesystem::path &path) {
    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        return std::get<6>(itm->second);
    }
    this->newFile(path);
    return isPermanent(path);
}

bool capiocl::Engine::isDirectory(const std::filesystem::path &path) {
    START_LOG(gettid(), "call(path=%s)", path.c_str());
    return !isFile(path);
}

void capiocl::Engine::setCommitedCloseNumber(const std::filesystem::path &path, const long num) {
    START_LOG(gettid(), "call(path=%s, num=%ld)", path.c_str(), num);
    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        std::get<7>(itm->second) = num;
    }
}

void capiocl::Engine::setDirectoryFileCount(const std::filesystem::path &path, long num) {
    START_LOG(gettid(), "call(path=%s, num=%ld)", path.c_str(), num);
    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        std::get<8>(itm->second) = num;
    }
}

void capiocl::Engine::remove(const std::filesystem::path &path) {
    START_LOG(gettid(), "call(path=%s)", path.c_str());
    _locations.erase(path);
}

std::vector<std::string> capiocl::Engine::getConsumers(const std::filesystem::path &path) {
    START_LOG(gettid(), "call(path=%s)", path.c_str());
    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        return std::get<1>(itm->second);
    }
    return {};
}

bool capiocl::Engine::isConsumer(const std::filesystem::path &path, const std::string &app_name) {
    START_LOG(gettid(), "call(path=%s, pid=%ld", path.c_str(), pid);

    LOG("App name is %s", app_name.c_str());

    // check for exact entry
    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        LOG("Found exact match for path");
        std::vector<std::string> producers = std::get<1>(itm->second);
        DBG(gettid(), [&](const std::vector<std::string> &arr) {
            for (auto elem : arr) {
                LOG("producer: %s", elem.c_str());
            }
        }(producers));
        return std::find(producers.begin(), producers.end(), app_name) != producers.end();
    }
    LOG("No exact match found in locations. checking for globs");

    // check for glob. Here we do not use the LMP check
    for (const auto &[k, entry] : _locations) {
        if (fnmatch(k.c_str(), path.c_str(), FNM_PATHNAME) == 0) {
            LOG("Found possible glob match");
            std::vector<std::string> producers = std::get<1>(entry);
            DBG(gettid(), [&](const std::vector<std::string> &arr) {
                for (auto itm : arr) {
                    LOG("producer: %s", itm.c_str());
                }
            }(producers));
            return std::find(producers.begin(), producers.end(), app_name) != producers.end();
        }
    }
    LOG("No match has been found");
    return false;
}

std::vector<std::string> capiocl::Engine::getProducers(const std::filesystem::path &path) {
    START_LOG(gettid(), "call(path=%s)", path.c_str());
    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        return std::get<0>(itm->second);
    }
    return {};
}

bool capiocl::Engine::isProducer(const std::filesystem::path &path, const std::string &app_name) {
    START_LOG(gettid(), "call(path=%s, pid=%ld", path.c_str(), pid);

    LOG("App name is %s", app_name.c_str());

    // check for exact entry
    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        LOG("Found exact match for path");
        std::vector<std::string> producers = std::get<0>(itm->second);
        DBG(gettid(), [&](const std::vector<std::string> &arr) {
            for (auto elem : arr) {
                LOG("producer: %s", elem.c_str());
            }
        }(producers));
        return std::find(producers.begin(), producers.end(), app_name) != producers.end();
    }
    LOG("No exact match found in locations. checking for globs");

    // check for glob. Here we do not use the LMP check
    for (const auto &[k, entry] : _locations) {
        if (fnmatch(k.c_str(), path.c_str(), FNM_PATHNAME) == 0) {
            LOG("Found possible glob match");
            std::vector<std::string> producers = std::get<0>(entry);
            DBG(gettid(), [&](const std::vector<std::string> &arr) {
                for (auto itm : arr) {
                    LOG("producer: %s", itm.c_str());
                }
            }(producers));
            return std::find(producers.begin(), producers.end(), app_name) != producers.end();
        }
    }
    LOG("No match has been found");
    return false;
}

void capiocl::Engine::setFileDeps(const std::filesystem::path &path,
                                  const std::vector<std::filesystem::path> &dependencies) {
    START_LOG(gettid(), "call()");
    if (dependencies.empty()) {
        return;
    }
    if (_locations.find(path) == _locations.end()) {
        this->newFile(path);
    }
    std::get<9>(_locations.at(path)) = dependencies;
    for (const auto &itm : dependencies) {
        LOG("Creating new fie (if it exists) for path %s", itm.c_str());
        newFile(itm);
    }
}

long capiocl::Engine::getCommitCloseCount(std::filesystem::path::iterator::reference path) const {
    START_LOG(gettid(), "call(path=%s)", path.c_str());
    long count = 0;
    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        count = std::get<7>(itm->second);
    }
    LOG("Expected number on close to commit file: %d", count);
    return count;
};

std::vector<std::filesystem::path>
capiocl::Engine::getCommitOnFileDependencies(const std::filesystem::path &path) {
    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        return std::get<9>(itm->second);
    }
    return {};
}

void capiocl::Engine::setStoreFileInMemory(const std::filesystem::path &path) {
    this->newFile(path);
    std::get<10>(_locations.at(path)) = true;
}
void capiocl::Engine::setAllStoreInMemory() {
    for (const auto &[fst, snd] : _locations) {
        this->setStoreFileInMemory(fst);
    }
}

void capiocl::Engine::setStoreFileInFileSystem(const std::filesystem::path &path) {
    this->newFile(path);
    std::get<10>(_locations.at(path)) = false;
}

bool capiocl::Engine::isStoredInMemory(const std::filesystem::path &path) {
    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        return std::get<10>(itm->second);
    }
    return false;
}

std::vector<std::filesystem::path> capiocl::Engine::getFileToStoreInMemory() {
    START_LOG(gettid(), "call()");
    std::vector<std::filesystem::path> files;

    for (const auto &[path, file] : _locations) {
        if (std::get<10>(file)) {
            files.push_back(path);
        }
    }

    return files;
}

std::string capiocl::Engine::getHomeNode(const std::filesystem::path &path) {
    // TODO: understand here how to get the home node policy when home_node_policies are
    //       being implemented.
    START_LOG(gettid(), "call(path=%s)", path.c_str());
    if (const auto location = _locations.find(path); location == _locations.end()) {
        LOG("No rule for home node. Returning create home node");
        return node_name;
    }

    LOG("Found location entry");
    return node_name;
}

bool capiocl::Engine::isExcluded(const std::filesystem::path &path) const {

    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        return std::get<5>(itm->second);
    }
    LOG("Checking against GLOB");

    size_t lpm_match_size = -1;
    bool lpm_match        = false;
    for (const auto &[glob_path, entry] : _locations) {
        if (fnmatch(glob_path.c_str(), path.c_str(), FNM_PATHNAME) == 0) {
            LOG("Found match with %s", glob_path.c_str());
            if (glob_path.string().length() > lpm_match_size) {
                lpm_match_size = glob_path.string().length();
                lpm_match      = std::get<5>(entry);
                LOG("Match is longer than previous match. storing value = %s",
                    lpm_match ? "true" : "false");
            }
        }
    }
    return lpm_match;
}
