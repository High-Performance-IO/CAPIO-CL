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
            map[full_key] = value.as_string()->get();
        }
    }
}

void capiocl::configuration::CapioClConfiguration::set(const std::string &key, std::string value) {
    config[key] = std::move(value);
}

void capiocl::configuration::CapioClConfiguration::load(const std::filesystem::path &path) {
    if (path.empty()) {
        return;
    }

    toml::table tbl;
    try {
        tbl = toml::parse_file(path.string());
    } catch (const toml::parse_error &err) {
        printer::print(printer::CLI_LEVEL_ERROR, err.what());
    }
    flatten_table(tbl, config);
}

void capiocl::configuration::CapioClConfiguration::getParameter(const std::string &key, int *value,
                                                                const int def_value) const {

    if (config.find(key) != config.end()) {
        *value = std::stoi(config.at(key));
    } else {
        *value = def_value;
    }
}

void capiocl::configuration::CapioClConfiguration::getParameter(const std::string &key,
                                                                std::string *value,
                                                                std::string def_value) const {
    if (config.find(key) != config.end()) {
        *value = config.at(key);
    } else {
        *value = std::move(def_value);
    }
}