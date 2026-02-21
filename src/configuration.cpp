#include <string>
#include <utility>

#include "capiocl/configuration.h"
#include "capiocl/printer.h"
#include "toml++/toml.hpp"

void load_config_to_memory(const toml::table &tbl,
                           std::unordered_map<std::string, std::string> &map,
                           const std::string &prefix = "") {
    for (const auto &[key, value] : tbl) {
        std::string full_key;
        if (prefix.empty()) {
            full_key = std::string{key.str()};
        } else {
            full_key = prefix + "." + std::string{key.str()};
        }

        if (value.is_table()) {
            load_config_to_memory(*value.as_table(), map, full_key);
        } else {
            if (value.is_string()) {
                map[full_key] = value.as_string()->get();
            } else if (value.is_boolean()) {
                if (value.as_boolean()->get()) {
                    map[full_key] = "true";
                } else {
                    map[full_key] = "false";
                }
            } else {
                map[full_key] = std::to_string(value.as_integer()->get());
            }
        }
    }
}

void capiocl::configuration::CapioClConfiguration::loadDefaults() {
    this->set(defaults::DEFAULT_MONITOR_MCAST_IP);
    this->set(defaults::DEFAULT_MONITOR_MCAST_PORT);
    this->set(defaults::DEFAULT_MONITOR_HOMENODE_IP);
    this->set(defaults::DEFAULT_MONITOR_HOMENODE_PORT);
    this->set(defaults::DEFAULT_MONITOR_MCAST_DELAY);
    this->set(defaults::DEFAULT_MONITOR_FS_ENABLED);
    this->set(defaults::DEFAULT_MONITOR_MCAST_ENABLED);
}

void capiocl::configuration::CapioClConfiguration::set(const std::string &key, std::string value) {
    config[key] = std::move(value);
}

void capiocl::configuration::CapioClConfiguration::set(const ConfigurationEntry &entry) {
    this->set(entry.k, entry.v);
}

void capiocl::configuration::CapioClConfiguration::load(const std::filesystem::path &path) {
    if (path.empty()) {
        throw CapioClConfigurationException("Empty pathname!");
    }

    toml::table tbl;
    try {
        tbl = toml::parse_file(path.string());
    } catch (const toml::parse_error &err) {
        throw CapioClConfigurationException(err.what());
    }

    // copy into the local configuration the parameter from the toml config file
    load_config_to_memory(tbl, config);
}

void capiocl::configuration::CapioClConfiguration::getParameter(const std::string &key,
                                                                int *value) const {

    if (config.find(key) != config.end()) {
        *value = std::stoi(config.at(key));
    } else {
        throw CapioClConfigurationException("Key " + key + " not found!");
    }
}

void capiocl::configuration::CapioClConfiguration::getParameter(const std::string &key,
                                                                std::string *value) const {
    if (config.find(key) != config.end()) {
        *value = config.at(key);
    } else {
        throw CapioClConfigurationException("Key " + key + " not found!");
    }
}
capiocl::configuration::CapioClConfigurationException::CapioClConfigurationException(
    const std::string &msg)
    : message(msg) {
    printer::print(printer::CLI_LEVEL_ERROR, msg);
}