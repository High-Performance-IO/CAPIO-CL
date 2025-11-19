#include "toml++/toml.hpp"
#include <string>
#include <utility>

#include "include/configuration.h"
#include "include/printer.h"

void flatten_table(const toml::table &tbl, std::unordered_map<std::string, std::string> &map,
                   const std::string &prefix = "") {
    for (const auto &[key, value] : tbl) {
        std::string full_key =
            prefix.empty() ? std::string{key.str()} : prefix + "." + std::string{key.str()};

        if (value.is_table()) {
            flatten_table(*value.as_table(), map, full_key);
        } else {
            const auto itm = value.as_string();
            if (itm == nullptr) {
                map[full_key] = std::to_string(value.as_integer()->get());
            } else {
                map[full_key] = itm->get();
            }
        }
    }
}

capiocl::configuration::CapioClConfiguration::CapioClConfiguration() {
    this->set(defaults::DEFAULT_MONITOR_MCAST_IP);
    this->set(defaults::DEFAULT_MONITOR_MCAST_PORT);
    this->set(defaults::DEFAULT_MONITOR_HOMENODE_IP);
    this->set(defaults::DEFAULT_MONITOR_HOMENODE_PORT);
    this->set(defaults::DEFAULT_MONITOR_MCAST_DELAY);
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
    flatten_table(tbl, config);
}

void capiocl::configuration::CapioClConfiguration::getParameter(const std::string &key,
                                                                int *value) const {

    if (config.find(key) != config.end()) {
        *value = std::stoi(config.at(key));
    } else {
        throw CapioClConfigurationException("Key" + key + " not found!");
    }
}

void capiocl::configuration::CapioClConfiguration::getParameter(const std::string &key,
                                                                std::string *value) const {
    if (config.find(key) != config.end()) {
        *value = config.at(key);
    } else {
        throw CapioClConfigurationException("Key" + key + " not found!");
    }
}
capiocl::configuration::CapioClConfigurationException::CapioClConfigurationException(
    const std::string &msg)
    : message(msg) {
    printer::print(printer::CLI_LEVEL_ERROR, msg);
}