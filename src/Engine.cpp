#include <algorithm>
#include <fnmatch.h>
#include <sstream>

#include "capiocl.hpp"
#include "capiocl/configuration.h"
#include "capiocl/engine.h"
#include "capiocl/monitor.h"
#include "capiocl/printer.h"

void capiocl::engine::Engine::print() const {
    // First message
    printer::print(printer::CLI_LEVEL_JSON, "");
    printer::print(printer::CLI_LEVEL_JSON, "Composition of expected CAPIO FS: ");

    // Table header lines
    printer::print(printer::CLI_LEVEL_JSON, "*" + std::string(134, '=') + "*");

    printer::print(printer::CLI_LEVEL_JSON, "|" + std::string(134, ' ') + "|");

    {
        std::ostringstream oss;
        oss << "|     Parsed configuration file for workflow: \033[1;36m" << workflow_name
            << std::setw(94 - workflow_name.length()) << "\033[0m |";
        printer::print(printer::CLI_LEVEL_JSON, oss.str());
    }

    printer::print(printer::CLI_LEVEL_JSON, "|" + std::string(134, ' ') + "|");

    std::string msg = "|     File color legend:     \033[48;5;034m  \033[0m File stored in memory";
    msg += std::string(82, ' ') + "|";
    printer::print(printer::CLI_LEVEL_JSON, msg);

    printer::print(
        printer::CLI_LEVEL_JSON, // LCOV_EXCL_LINE
        "|                            \033[48;5;172m  \033[0m File stored on file system" +
            std::string(77, ' ') + "|");

    printer::print(printer::CLI_LEVEL_JSON, "|" + std::string(134, '=') + "|");

    std::string line = "|======|===================|===================|====================";
    line += "|========|============|============|===========|=========|==========|";
    printer::print(printer::CLI_LEVEL_JSON, line);

    line = "| Kind | Filename          | Producer step     | Consumer step      |  ";
    line += "Commit Rule       |  Fire Rule | Permanent | Exclude | n_files  |";
    printer::print(printer::CLI_LEVEL_JSON, line);

    line = "|======|===================|===================|====================|========";
    line += "============|============|===========|=========|==========|";
    printer::print(printer::CLI_LEVEL_JSON, line);

    // Iterate over _locations
    for (auto &itm : _capio_cl_entries) {
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
        base_line << "|   " << color_preamble << kind << color_post << "  | " << color_preamble;
        base_line << name_trunc << color_post << std::setfill(' ');
        base_line << std::setw(20 - name_trunc.length()) << "| ";

        auto producers = itm.second.producers;
        auto consumers = itm.second.consumers;
        auto rowCount  = std::max(producers.size(), consumers.size());

        std::string n_files = std::to_string(itm.second.directory_children_count);
        if (itm.second.directory_children_count < 1) {
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

            printer::print(printer::CLI_LEVEL_JSON, line.str());
        }

        printer::print(printer::CLI_LEVEL_JSON, "*" + std::string(134, '~') + "*");
    }

    printer::print(printer::CLI_LEVEL_JSON, "");
}

capiocl::engine::Engine::Engine(const bool use_default_settings) {
    node_name = std::string(1024, '\0');
    gethostname(node_name.data(), node_name.size());
    node_name.resize(std::strlen(node_name.c_str()));

    if (const char *_wf_name = std::getenv("WORKFLOW_NAME"); _wf_name != nullptr) {
        this->workflow_name = _wf_name;
    } else {
        this->workflow_name = CAPIO_CL_DEFAULT_WF_NAME;
    }

    if (use_default_settings) {
        this->useDefaultConfiguration();
    }
}

void capiocl::engine::Engine::_newFile(const std::filesystem::path &path) const {
    if (path.empty()) {
        return;
    }

    if (_capio_cl_entries.find(path) == _capio_cl_entries.end()) {
        std::string commit = commitRules::ON_TERMINATION;
        std::string fire   = fireRules::UPDATE;

        std::string matchKey;
        size_t matchSize = 0;
        for (const auto &[filename, data] : _capio_cl_entries) {
            if (const bool match = fnmatch(filename.c_str(), path.c_str(), FNM_NOESCAPE) == 0;
                match && filename.length() > matchSize) {
                matchSize = filename.length();
                matchKey  = filename;
            }
        }

        CapioCLEntry entry;
        entry.commit_rule = commit;
        entry.fire_rule   = fire;

        if (matchSize > 0) {
            const auto &data = _capio_cl_entries.at(matchKey);

            // Duplicate CapioCLEntry object and register it to new resolved path
            // This is achieved by not using & operator
            entry = data;
            if (store_all_in_memory) {
                entry.store_in_memory = true;
            } else {
                entry.store_in_memory = data.store_in_memory;
            }
        } else {
            entry.store_in_memory = store_all_in_memory;
        }
        _capio_cl_entries.emplace(path, std::move(entry));
        this->compute_directory_entry_count(path);
    }
}

void capiocl::engine::Engine::compute_directory_entry_count(
    const std::filesystem::path &path) const {
    if (const auto parent = path.parent_path(); !parent.empty()) {
        if (const auto &entry = _capio_cl_entries.find(parent); entry != _capio_cl_entries.end()) {
            if (entry->second.enable_directory_count_update) {
                entry->second.directory_children_count++;
                entry->second.is_file = false;
            } else {
                return;
            }
        }
    } else {
        return;
    }
}

bool capiocl::engine::Engine::contains(const std::filesystem::path &file) const {
    return std::any_of(_capio_cl_entries.begin(), _capio_cl_entries.end(), [&](auto const &entry) {
        return fnmatch(entry.first.c_str(), file.c_str(), FNM_NOESCAPE) == 0;
    });
}

size_t capiocl::engine::Engine::size() const { return this->_capio_cl_entries.size(); }

void capiocl::engine::Engine::add(std::filesystem::path &path, std::vector<std::string> &producers,
                                  std::vector<std::string> &consumers,
                                  const std::string &commit_rule, const std::string &fire_rule,
                                  bool permanent, bool exclude,
                                  std::vector<std::filesystem::path> &dependencies) {
    if (path.empty()) {
        return;
    }

    this->_newFile(path);

    CapioCLEntry &entry     = _capio_cl_entries.at(path);
    entry.producers         = producers;
    entry.consumers         = consumers;
    entry.commit_rule       = commit_rule;
    entry.fire_rule         = fire_rule;
    entry.permanent         = permanent;
    entry.excluded          = exclude;
    entry.file_dependencies = dependencies;
}

void capiocl::engine::Engine::newFile(const std::filesystem::path &path) { this->_newFile(path); }

long capiocl::engine::Engine::getDirectoryFileCount(const std::filesystem::path &path) const {
    if (path.empty()) {
        return 0;
    }

    if (const auto itm = _capio_cl_entries.find(path); itm != _capio_cl_entries.end()) {
        return itm->second.directory_children_count;
    }
    this->_newFile(path);
    return getDirectoryFileCount(path);
}

void capiocl::engine::Engine::addProducer(const std::filesystem::path &path,
                                          std::string &producer) {
    if (path.empty()) {
        return;
    }

    producer.erase(remove_if(producer.begin(), producer.end(), isspace), producer.end());

    if (const auto itm = _capio_cl_entries.find(path); itm != _capio_cl_entries.end()) {
        auto &vec = itm->second.producers;
        if (std::find(vec.begin(), vec.end(), producer) == vec.end()) {
            vec.emplace_back(producer);
            return;
        } else {
            return;
        }
    }
    this->newFile(path);
    this->addProducer(path, producer);
}

void capiocl::engine::Engine::addConsumer(const std::filesystem::path &path,
                                          std::string &consumer) {
    if (path.empty()) {
        return;
    }

    consumer.erase(remove_if(consumer.begin(), consumer.end(), isspace), consumer.end());
    if (const auto itm = _capio_cl_entries.find(path); itm != _capio_cl_entries.end()) {
        auto &vec = itm->second.consumers;
        if (std::find(vec.begin(), vec.end(), consumer) == vec.end()) {
            vec.emplace_back(consumer);
        }
        return;
    }
    this->newFile(path);
    this->addConsumer(path, consumer);
}

void capiocl::engine::Engine::addFileDependency(const std::filesystem::path &path,
                                                std::filesystem::path &file_dependency) {
    if (path.empty()) {
        return;
    }

    if (const auto itm = _capio_cl_entries.find(path); itm != _capio_cl_entries.end()) {
        auto &vec = itm->second.file_dependencies;
        if (std::find(vec.begin(), vec.end(), file_dependency) == vec.end()) {
            vec.emplace_back(file_dependency);
        }
        return;
    }
    this->newFile(path);
    this->setCommitRule(path, commitRules::ON_FILE);
    this->addFileDependency(path, file_dependency);
}

void capiocl::engine::Engine::setCommitRule(const std::filesystem::path &path,
                                            const std::string &commit_rule) {
    if (path.empty()) {
        return;
    }

    const auto commit = commitRules::sanitize(commit_rule);

    if (const auto itm = _capio_cl_entries.find(path); itm != _capio_cl_entries.end()) {
        itm->second.commit_rule = commit;
        return;
    }
    this->_newFile(path);
    this->setCommitRule(path, commit);
}

std::string capiocl::engine::Engine::getCommitRule(const std::filesystem::path &path) const {
    if (path.empty()) {
        return commitRules::ON_TERMINATION;
    }

    if (const auto itm = _capio_cl_entries.find(path); itm != _capio_cl_entries.end()) {
        return itm->second.commit_rule;
    }

    this->_newFile(path);
    return getCommitRule(path);
}

std::string capiocl::engine::Engine::getFireRule(const std::filesystem::path &path) const {
    if (path.empty()) {
        return fireRules::NO_UPDATE;
    }

    if (const auto itm = _capio_cl_entries.find(path); itm != _capio_cl_entries.end()) {
        return itm->second.fire_rule;
    }

    this->_newFile(path);
    return getFireRule(path);
}

void capiocl::engine::Engine::setFireRule(const std::filesystem::path &path,
                                          const std::string &fire_rule) {
    if (path.empty()) {
        return;
    }

    const auto fire = fireRules::sanitize(fire_rule);

    if (const auto itm = _capio_cl_entries.find(path); itm != _capio_cl_entries.end()) {
        itm->second.fire_rule = fire;
        return;
    }
    this->_newFile(path);
    setFireRule(path, fire);
}

bool capiocl::engine::Engine::isFirable(const std::filesystem::path &path) const {
    if (path.empty()) {
        return true;
    }

    if (const auto itm = _capio_cl_entries.find(path); itm != _capio_cl_entries.end()) {
        return itm->second.fire_rule == fireRules::NO_UPDATE;
    }

    this->_newFile((path));
    return isFirable(path);
}

void capiocl::engine::Engine::setPermanent(const std::filesystem::path &path, bool value) {
    if (path.empty()) {
        return;
    }

    if (const auto itm = _capio_cl_entries.find(path); itm != _capio_cl_entries.end()) {
        itm->second.permanent = value;
        return;
    }
    this->_newFile(path);
    setPermanent(path, value);
}

bool capiocl::engine::Engine::isPermanent(const std::filesystem::path &path) const {
    if (path.empty()) {
        return true;
    }

    if (const auto itm = _capio_cl_entries.find(path); itm != _capio_cl_entries.end()) {
        return itm->second.permanent;
    }

    this->_newFile(path);
    return isPermanent(path);
}

bool capiocl::engine::Engine::isCommitted(const std::filesystem::path &path) const {
    return monitor.isCommitted(path);
}

void capiocl::engine::Engine::setCommitted(const std::filesystem::path &path) const {
    monitor.setCommitted(path);
}

std::vector<std::string> capiocl::engine::Engine::getPaths() const {
    std::vector<std::string> paths;
    for (const auto &[k, v] : _capio_cl_entries) {
        paths.push_back(k);
    }
    return paths;
}

void capiocl::engine::Engine::setExclude(const std::filesystem::path &path, const bool value) {
    if (path.empty()) {
        return;
    }

    if (const auto itm = _capio_cl_entries.find(path); itm != _capio_cl_entries.end()) {
        itm->second.excluded = value;
        return;
    }
    this->_newFile(path);
    setExclude(path, value);
}

void capiocl::engine::Engine::setDirectory(const std::filesystem::path &path) {
    if (path.empty()) {
        return;
    }

    if (const auto itm = _capio_cl_entries.find(path); itm != _capio_cl_entries.end()) {
        itm->second.is_file = false;
        return;
    }
    this->_newFile(path);
    setDirectory(path);
}

void capiocl::engine::Engine::setFile(const std::filesystem::path &path) {
    if (path.empty()) {
        return;
    }

    if (const auto itm = _capio_cl_entries.find(path); itm != _capio_cl_entries.end()) {
        itm->second.is_file = true;
        return;
    }
    this->_newFile(path);
    setFile(path);
}

bool capiocl::engine::Engine::isFile(const std::filesystem::path &path) const {
    if (path.empty()) {
        return true;
    }

    if (const auto itm = _capio_cl_entries.find(path); itm != _capio_cl_entries.end()) {
        return itm->second.is_file;
    }
    this->_newFile(path);
    return isPermanent(path);
}

bool capiocl::engine::Engine::isDirectory(const std::filesystem::path &path) const {
    if (path.empty()) {
        return true;
    }

    return !isFile(path);
}

void capiocl::engine::Engine::setCommitedCloseNumber(const std::filesystem::path &path,
                                                     const long num) {
    if (path.empty()) {
        return;
    }

    if (const auto itm = _capio_cl_entries.find(path); itm != _capio_cl_entries.end()) {
        itm->second.commit_on_close_count = num;
        return;
    }
    this->_newFile(path);
    setCommitedCloseNumber(path, num);
}

void capiocl::engine::Engine::setDirectoryFileCount(const std::filesystem::path &path,
                                                    const long num) {
    if (path.empty()) {
        return;
    }

    if (const auto itm = _capio_cl_entries.find(path); itm != _capio_cl_entries.end()) {
        this->setDirectory(path);
        itm->second.directory_children_count      = num;
        itm->second.enable_directory_count_update = false;
        return;
    }
    this->_newFile(path);
    this->setDirectoryFileCount(path, num);
}

void capiocl::engine::Engine::remove(const std::filesystem::path &path) const {
    if (const auto itm = _capio_cl_entries.find(path); itm == _capio_cl_entries.end()) {
        return;
    }
    _capio_cl_entries.erase(path);
}

std::vector<std::string>
capiocl::engine::Engine::getConsumers(const std::filesystem::path &path) const {
    if (const auto itm = _capio_cl_entries.find(path); itm != _capio_cl_entries.end()) {
        return itm->second.consumers;
    }
    return {};
}

bool capiocl::engine::Engine::isConsumer(const std::filesystem::path &path,
                                         const std::string &app_name) const {
    if (path.empty()) {
        return true;
    }

    for (const auto &[pattern, entry] : _capio_cl_entries) {
        if (fnmatch(pattern.c_str(), path.c_str(), FNM_NOESCAPE) == 0) {
            const auto &consumers = entry.consumers;
            if (std::find(consumers.begin(), consumers.end(), app_name) != consumers.end()) {
                return true;
            }
        }
    }

    this->_newFile(path);
    return false;
}

std::vector<std::string>
capiocl::engine::Engine::getProducers(const std::filesystem::path &path) const {
    if (path.empty()) {
        return {};
    }

    if (const auto itm = _capio_cl_entries.find(path); itm != _capio_cl_entries.end()) {
        return itm->second.producers;
    }
    this->_newFile(path);
    return getProducers(path);
}

bool capiocl::engine::Engine::isProducer(const std::filesystem::path &path,
                                         const std::string &app_name) const {
    if (path.empty()) {
        return true;
    }

    for (const auto &[pattern, entry] : _capio_cl_entries) {
        if (fnmatch(pattern.c_str(), path.c_str(), FNM_NOESCAPE) == 0) {
            const auto &producers = entry.producers;
            if (std::find(producers.begin(), producers.end(), app_name) != producers.end()) {
                return true;
            }
        }
    }

    this->_newFile(path);
    return false;
}

void capiocl::engine::Engine::setFileDeps(const std::filesystem::path &path,
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

    if (const auto itm = _capio_cl_entries.find(path); itm != _capio_cl_entries.end()) {
        itm->second.file_dependencies = dependencies;
        return;
    }
    this->_newFile(path);
    setFileDeps(path, dependencies);
}

long capiocl::engine::Engine::getCommitCloseCount(
    const std::filesystem::path::iterator::reference &path) const {
    if (path.empty()) {
        return 0;
    }

    if (const auto itm = _capio_cl_entries.find(path); itm != _capio_cl_entries.end()) {
        return itm->second.commit_on_close_count;
    }

    this->_newFile(path);
    return getCommitCloseCount(path);
}

std::vector<std::filesystem::path>
capiocl::engine::Engine::getCommitOnFileDependencies(const std::filesystem::path &path) const {
    if (const auto itm = _capio_cl_entries.find(path); itm != _capio_cl_entries.end()) {
        return itm->second.file_dependencies;
    }
    return {};
}

void capiocl::engine::Engine::setStoreFileInMemory(const std::filesystem::path &path) {
    if (path.empty()) {
        return;
    }

    if (const auto itm = _capio_cl_entries.find(path); itm != _capio_cl_entries.end()) {
        itm->second.store_in_memory = true;
        return;
    }
    this->_newFile(path);
    setStoreFileInMemory(path);
}

void capiocl::engine::Engine::setAllStoreInMemory() {
    this->store_all_in_memory = true;
    for (const auto &[fst, snd] : _capio_cl_entries) {
        this->setStoreFileInMemory(fst);
    }
}

void capiocl::engine::Engine::setWorkflowName(const std::string &name) {
    this->workflow_name = name;
}

const std::string &capiocl::engine::Engine::getWorkflowName() const { return this->workflow_name; }

void capiocl::engine::Engine::setStoreFileInFileSystem(const std::filesystem::path &path) {
    if (path.empty()) {
        return;
    }

    if (const auto itm = _capio_cl_entries.find(path); itm != _capio_cl_entries.end()) {
        itm->second.store_in_memory = false;
        return;
    }
    this->_newFile(path);
    setStoreFileInFileSystem(path);
}

bool capiocl::engine::Engine::isStoredInMemory(const std::filesystem::path &path) const {
    if (path.empty()) {
        return true;
    }

    if (const auto itm = _capio_cl_entries.find(path); itm != _capio_cl_entries.end()) {
        return itm->second.store_in_memory;
    }

    this->_newFile(path);
    return isStoredInMemory(path);
}

std::vector<std::string> capiocl::engine::Engine::getFileToStoreInMemory() const {
    std::vector<std::string> files;

    for (const auto &[path, file] : _capio_cl_entries) {
        if (file.store_in_memory) {
            files.push_back(path);
        }
    }

    return files;
}

std::set<std::string>
capiocl::engine::Engine::getHomeNode(const std::filesystem::path &path) const {
    return monitor.getHomeNode(path);
}

void capiocl::engine::Engine::setHomeNode(const std::filesystem::path &path) const {
    monitor.setHomeNode(path);
}

bool capiocl::engine::Engine::isExcluded(const std::filesystem::path &path) const {
    if (path.empty()) {
        return true;
    }

    if (const auto itm = _capio_cl_entries.find(path); itm != _capio_cl_entries.end()) {
        return itm->second.excluded;
    }

    this->_newFile(path);
    return isExcluded(path);
}

bool capiocl::engine::Engine::operator==(const capiocl::engine::Engine &other) const {
    const auto &other_entries = other._capio_cl_entries;

    if (this->_capio_cl_entries.size() != other_entries.size()) {
        return false;
    }

    for (const auto &[this_path, this_itm] : this->_capio_cl_entries) {
        if (other_entries.find(this_path) == other_entries.end()) {
            return false;
        }
        auto other_itm = other_entries.at(this_path);

        if (this_itm.commit_rule != other_itm.commit_rule ||
            this_itm.fire_rule != other_itm.fire_rule ||
            this_itm.permanent != other_itm.permanent || this_itm.excluded != other_itm.excluded ||
            this_itm.is_file != other_itm.is_file ||
            this_itm.commit_on_close_count != other_itm.commit_on_close_count ||
            this_itm.directory_children_count != other_itm.directory_children_count ||
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
void capiocl::engine::Engine::loadConfiguration(const std::string &path) {
    configuration.load(path);

    monitor.registerMonitorBackend(new monitor::MulticastMonitor(configuration));
    monitor.registerMonitorBackend(new monitor::FileSystemMonitor());
}
void capiocl::engine::Engine::useDefaultConfiguration() {

    const auto def_config = configuration::CapioClConfiguration();

    monitor.registerMonitorBackend(new monitor::MulticastMonitor(def_config));
    monitor.registerMonitorBackend(new monitor::FileSystemMonitor());
}

void capiocl::engine::Engine::startApiServer(const std::string &address, const int port) {
    webapi_server = std::unique_ptr<webapi::CapioClWebApiServer>(
        new webapi::CapioClWebApiServer(this, address, port));
}