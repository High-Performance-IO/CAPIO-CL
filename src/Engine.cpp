#include "capiocl.hpp"
#include <algorithm>
#include <fnmatch.h>
#include <iostream>
#include <sstream>

void capiocl::Engine::print() const {
    // First message
    print_message(CLI_LEVEL_JSON, "");
    print_message(CLI_LEVEL_JSON, "Composition of expected CAPIO FS: ");

    // Table header lines
    print_message(CLI_LEVEL_JSON, "*" + std::string(134, '=') + "*");

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

    print_message(CLI_LEVEL_JSON, "|" + std::string(134, '=') + "|");

    std::string line = "|======|===================|===================|====================";
    line += "|========|============|============|===========|=========|==========|";
    print_message(CLI_LEVEL_JSON, line);

    line = "| Kind | Filename          | Producer step     | Consumer step      |  ";
    line += "Commit Rule       |  Fire Rule | Permanent | Exclude | n_files  |";
    print_message(CLI_LEVEL_JSON, line);

    line = "|======|===================|===================|====================|========";
    line += "============|============|===========|=========|==========|";
    print_message(CLI_LEVEL_JSON, line);

    // Iterate over _locations
    for (auto &itm : _locations) {
        std::string color_preamble =
            itm.second.store_in_memory ? "\033[38;5;034m" : "\033[38;5;172m";
        std::string color_post = "\033[0m";

        std::string name_trunc = truncateLastN(itm.first, 12);
        std::string kind;
        if (itm.second.is_file) {
            kind = "F";
        } else {
            kind = "D";
        }

        std::ostringstream base_line;
        base_line << "|   " << color_preamble << kind << color_post << "  | " << color_preamble
                  << name_trunc << color_post << std::setfill(' ')
                  << std::setw(20 - name_trunc.length()) << "| ";

        auto producers = itm.second.producers;
        auto consumers = itm.second.consumers;
        auto rowCount  = std::max(producers.size(), consumers.size());

        std::string n_files = std::to_string(itm.second.directory_commit_file_count);
        if (itm.second.directory_commit_file_count < 1) {
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
                std::string commit_rule = itm.second.commit_rule, fire_rule = itm.second.fire_rule;
                bool exclude = itm.second.excluded, permanent = itm.second.permanent;

                line << " " << commit_rule << std::setfill(' ');
                line << std::setw(20 - commit_rule.length()) << " | " << fire_rule;
                line << std::setfill(' ') << std::setw(13 - fire_rule.length()) << " | ";
                line << "    " << (permanent ? "YES" : "NO ") << "   |   ";
                line << (exclude ? "YES" : "NO ");
                line << "   | " << n_files << std::setw(10 - n_files.length()) << " |";
            } else {
                line << std::setfill(' ') << std::setw(20) << "|" << std::setfill(' ');
                line << std::setw(13) << "|" << std::setfill(' ') << std::setw(12) << "|";
                line << std::setfill(' ') << std::setw(10) << "|" << std::setw(11) << "|";
            }

            print_message(CLI_LEVEL_JSON, line.str());
        }

        print_message(CLI_LEVEL_JSON, "*" + std::string(134, '~') + "*");
    }

    print_message(CLI_LEVEL_JSON, "");
}

bool capiocl::Engine::contains(const std::filesystem::path &file) {
    for (auto &[fst, snd] : _locations) {
        if (fnmatch(fst.c_str(), file.c_str(), FNM_PATHNAME) == 0) {
            return true;
        }
    }
    return false;
}
size_t capiocl::Engine::size() const { return this->_locations.size(); }

void capiocl::Engine::add(std::filesystem::path &path, std::vector<std::string> &producers,
                          std::vector<std::string> &consumers, const std::string &commit_rule,
                          const std::string &fire_rule, bool permanent, bool exclude,
                          std::vector<std::filesystem::path> &dependencies) {
    if (path.empty()) {
        return;
    }

    if (_locations.find(path) == _locations.end()) {
        CapioCLEntry entry;
        entry.producers         = producers;
        entry.consumers         = consumers;
        entry.commit_rule       = commit_rule;
        entry.fire_rule         = fire_rule;
        entry.permanent         = permanent;
        entry.excluded          = exclude;
        entry.file_dependencies = dependencies;
        _locations.emplace(path, entry);
    }
}

void capiocl::Engine::newFile(const std::filesystem::path &path) {

    if (path.empty()) {
        return;
    }

    if (_locations.find(path) == _locations.end()) {
        std::string commit = commit_rules::ON_TERMINATION;
        std::string fire   = fire_rules::UPDATE;

        std::string matchKey;
        size_t matchSize = 0;
        for (const auto &[filename, data] : _locations) {
            if (const bool match = fnmatch(filename.c_str(), path.c_str(), FNM_PATHNAME) == 0;
                match && filename.length() > matchSize) {
                matchSize = filename.length();
                matchKey  = filename;
            }
        }

        if (matchSize > 0) {
            const auto &data = _locations.at(matchKey);
            _locations.emplace(
                path, CapioCLEntry{data.producers, data.consumers, data.file_dependencies,
                                   data.commit_rule, data.fire_rule, data.permanent, data.excluded,
                                   data.is_file, data.store_in_memory || store_all_in_memory,
                                   data.commit_on_close_count, data.directory_commit_file_count});

        } else {
            _locations.emplace(path, CapioCLEntry());
        }
    }
}

long capiocl::Engine::getDirectoryFileCount(const std::filesystem::path &path) {

    if (path.empty()) {
        return 0;
    }

    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        return itm->second.directory_commit_file_count;
    }
    this->newFile(path);
    return getDirectoryFileCount(path);
}

void capiocl::Engine::addProducer(const std::filesystem::path &path, std::string &producer) {

    if (path.empty()) {
        return;
    }

    producer.erase(remove_if(producer.begin(), producer.end(), isspace), producer.end());

    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        auto &vec = itm->second.producers;
        if (std::find(vec.begin(), vec.end(), producer) == vec.end()) {
            vec.emplace_back(producer);
        }
        return;
    }
    this->newFile(path);
    this->addProducer(path, producer);
}

void capiocl::Engine::addConsumer(const std::filesystem::path &path, std::string &consumer) {

    if (path.empty()) {
        return;
    }

    consumer.erase(remove_if(consumer.begin(), consumer.end(), isspace), consumer.end());
    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        auto &vec = itm->second.consumers;
        if (std::find(vec.begin(), vec.end(), consumer) == vec.end()) {
            vec.emplace_back(consumer);
        }
        return;
    }
    this->newFile(path);
    this->addConsumer(path, consumer);
}

void capiocl::Engine::addFileDependency(const std::filesystem::path &path,
                                        std::filesystem::path &file_dependency) {

    if (path.empty()) {
        return;
    }

    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        auto &vec = itm->second.file_dependencies;
        if (std::find(vec.begin(), vec.end(), file_dependency) == vec.end()) {
            vec.emplace_back(file_dependency);
        }
        return;
    }
    this->newFile(path);
    this->setCommitRule(path, commit_rules::ON_FILE);
    this->addFileDependency(path, file_dependency);
}

void capiocl::Engine::setCommitRule(const std::filesystem::path &path,
                                    const std::string &commit_rule) {

    if (path.empty()) {
        return;
    }

    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        itm->second.commit_rule = commit_rule;
        return;
    }
    this->newFile(path);
    this->setCommitRule(path, commit_rule);
}

std::string capiocl::Engine::getCommitRule(const std::filesystem::path &path) {

    if (path.empty()) {
        return commit_rules::ON_TERMINATION;
    }

    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        return itm->second.commit_rule;
    }

    this->newFile(path);
    return getCommitRule(path);
}

std::string capiocl::Engine::getFireRule(const std::filesystem::path &path) {

    if (path.empty()) {
        return fire_rules::NO_UPDATE;
    }

    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        return itm->second.fire_rule;
    }

    this->newFile(path);
    return getFireRule(path);
}

void capiocl::Engine::setFireRule(const std::filesystem::path &path, const std::string &fire_rule) {

    if (path.empty()) {
        return;
    }

    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        itm->second.fire_rule = fire_rule;
        return;
    }
    this->newFile(path);
    setFireRule(path, fire_rule);
}

bool capiocl::Engine::isFirable(const std::filesystem::path &path) {

    if (path.empty()) {
        return true;
    }

    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        return itm->second.fire_rule == fire_rules::NO_UPDATE;
    }

    this->newFile((path));
    return isFirable(path);
}

void capiocl::Engine::setPermanent(const std::filesystem::path &path, bool value) {

    if (path.empty()) {
        return;
    }

    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        itm->second.permanent = value;
        return;
    }
    this->newFile(path);
    setPermanent(path, value);
}

bool capiocl::Engine::isPermanent(const std::filesystem::path &path) {

    if (path.empty()) {
        return true;
    }

    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        return itm->second.permanent;
    }

    this->newFile(path);
    return isPermanent(path);
}

void capiocl::Engine::setExclude(const std::filesystem::path &path, const bool value) {

    if (path.empty()) {
        return;
    }

    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        itm->second.excluded = value;
        return;
    }
    this->newFile(path);
    setExclude(path, value);
}

void capiocl::Engine::setDirectory(const std::filesystem::path &path) {

    if (path.empty()) {
        return;
    }

    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        itm->second.is_file = false;
        return;
    }
    this->newFile(path);
    setDirectory(path);
}

void capiocl::Engine::setFile(const std::filesystem::path &path) {

    if (path.empty()) {
        return;
    }

    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        itm->second.is_file = true;
        return;
    }
    this->newFile(path);
    setFile(path);
}

bool capiocl::Engine::isFile(const std::filesystem::path &path) {

    if (path.empty()) {
        return true;
    }

    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        return itm->second.is_file;
    }
    this->newFile(path);
    return isPermanent(path);
}

bool capiocl::Engine::isDirectory(const std::filesystem::path &path) {

    if (path.empty()) {
        return true;
    }

    return !isFile(path);
}

void capiocl::Engine::setCommitedCloseNumber(const std::filesystem::path &path, const long num) {

    if (path.empty()) {
        return;
    }

    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        itm->second.commit_on_close_count = num;
        return;
    }
    this->newFile(path);
    setCommitedCloseNumber(path, num);
}

void capiocl::Engine::setDirectoryFileCount(const std::filesystem::path &path, long num) {

    if (path.empty()) {
        return;
    }

    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        itm->second.directory_commit_file_count = num;
        return;
    }
    this->newFile(path);
    this->setDirectory(path);
    this->setDirectoryFileCount(path, num);
}

void capiocl::Engine::remove(const std::filesystem::path &path) {
    if (const auto itm = _locations.find(path); itm == _locations.end()) {
        return;
    }
    _locations.erase(path);
}

std::vector<std::string> capiocl::Engine::getConsumers(const std::filesystem::path &path) {
    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        return itm->second.consumers;
    }
    return {};
}

bool capiocl::Engine::isConsumer(const std::filesystem::path &path, const std::string &app_name) {
    if (path.empty()) {
        return true;
    }

    for (const auto &[pattern, entry] : _locations) {
        if (fnmatch(pattern.c_str(), path.c_str(), FNM_PATHNAME) == 0) {
            const auto &consumers = entry.consumers;
            if (std::find(consumers.begin(), consumers.end(), app_name) != consumers.end()) {
                return true;
            }
        }
    }

    this->newFile(path);
    return false;
}

std::vector<std::string> capiocl::Engine::getProducers(const std::filesystem::path &path) {

    if (path.empty()) {
        return {};
    }

    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        return itm->second.producers;
    }
    this->newFile(path);
    return getProducers(path);
}

bool capiocl::Engine::isProducer(const std::filesystem::path &path, const std::string &app_name) {
    if (path.empty()) {
        return true;
    }

    for (const auto &[pattern, entry] : _locations) {
        if (fnmatch(pattern.c_str(), path.c_str(), FNM_PATHNAME) == 0) {
            const auto &producers = entry.producers;
            if (std::find(producers.begin(), producers.end(), app_name) != producers.end()) {
                return true;
            }
        }
    }

    this->newFile(path);
    return false;
}

void capiocl::Engine::setFileDeps(const std::filesystem::path &path,
                                  const std::vector<std::filesystem::path> &dependencies) {

    if (path.empty()) {
        return;
    }

    if (dependencies.empty()) {
        return;
    }

    for (const auto &itm : dependencies) {
        newFile(itm);
    }

    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        itm->second.file_dependencies = dependencies;
        return;
    }
    this->newFile(path);
    setFileDeps(path, dependencies);
}

long capiocl::Engine::getCommitCloseCount(std::filesystem::path::iterator::reference path) {

    if (path.empty()) {
        return 0;
    }

    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        return itm->second.commit_on_close_count;
    }

    this->newFile(path);
    return getCommitCloseCount(path);
}

std::vector<std::filesystem::path>
capiocl::Engine::getCommitOnFileDependencies(const std::filesystem::path &path) {
    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        return itm->second.file_dependencies;
    }
    return {};
}

void capiocl::Engine::setStoreFileInMemory(const std::filesystem::path &path) {

    if (path.empty()) {
        return;
    }

    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        itm->second.store_in_memory = true;
        return;
    }
    this->newFile(path);
    setStoreFileInMemory(path);
}

void capiocl::Engine::setAllStoreInMemory() {
    this->store_all_in_memory = true;
    for (const auto &[fst, snd] : _locations) {
        this->setStoreFileInMemory(fst);
    }
}

void capiocl::Engine::setStoreFileInFileSystem(const std::filesystem::path &path) {

    if (path.empty()) {
        return;
    }

    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        itm->second.store_in_memory = false;
        return;
    }
    this->newFile(path);
    setStoreFileInFileSystem(path);
}

bool capiocl::Engine::isStoredInMemory(const std::filesystem::path &path) {

    if (path.empty()) {
        return true;
    }

    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        return itm->second.store_in_memory;
    }

    this->newFile(path);
    return isStoredInMemory(path);
}

std::vector<std::string> capiocl::Engine::getFileToStoreInMemory() {
    std::vector<std::string> files;

    for (const auto &[path, file] : _locations) {
        if (file.store_in_memory) {
            files.push_back(path);
        }
    }

    return files;
}

std::string capiocl::Engine::getHomeNode(const std::filesystem::path &path) {

    if (const auto location = _locations.find(path); location == _locations.end()) {
        return node_name;
    }
    return node_name;
}

bool capiocl::Engine::isExcluded(const std::filesystem::path &path) {

    if (path.empty()) {
        return true;
    }

    if (const auto itm = _locations.find(path); itm != _locations.end()) {
        return itm->second.excluded;
    }

    this->newFile(path);
    return isExcluded(path);
}

bool capiocl::Engine::operator==(const capiocl::Engine &other) const {
    const auto &other_locations = other.getLocations();

    if (this->_locations.size() != other_locations->size()) {
        return false;
    }

    for (const auto &[this_path, this_itm] : this->_locations) {
        if (other_locations->find(this_path) == other_locations->end()) {
            return false;
        }
        auto other_itm = other_locations->at(this_path);

        if (this_itm.commit_rule != other_itm.commit_rule ||
            this_itm.fire_rule != other_itm.fire_rule ||
            this_itm.permanent != other_itm.permanent || this_itm.excluded != other_itm.excluded ||
            this_itm.is_file != other_itm.is_file ||
            this_itm.commit_on_close_count != other_itm.commit_on_close_count ||
            this_itm.directory_commit_file_count != other_itm.directory_commit_file_count ||
            this_itm.store_in_memory != other_itm.store_in_memory) {
            return false;
        }

        auto this_producer  = this_itm.producers;
        auto other_producer = other_itm.producers;
        if (this_producer.size() != other_producer.size()) {
            return false;
        }
        for (const auto &entry : this_producer) {
            if (std::find(other_producer.begin(), other_producer.end(), entry) ==
                other_producer.end()) {
                return false;
            }
        }

        auto this_consumer  = this_itm.consumers;
        auto other_consumer = other_itm.consumers;
        if (this_consumer.size() != other_consumer.size()) {
            return false;
        }
        for (const auto &entry : this_consumer) {
            if (std::find(other_consumer.begin(), other_consumer.end(), entry) ==
                other_consumer.end()) {
                return false;
            }
        }

        auto this_deps  = this_itm.file_dependencies;
        auto other_deps = other_itm.file_dependencies;
        if (this_deps.size() != other_deps.size()) {
            return false;
        }
        for (const auto &entry : this_deps) {
            if (std::find(other_deps.begin(), other_deps.end(), entry) == other_deps.end()) {
                return false;
            }
        }
    }
    return true;
}
