#ifndef CAPIO_CL_CONFIGURATION_H
#define CAPIO_CL_CONFIGURATION_H
#include <filesystem>
#include <unordered_map>

#include "capiocl.hpp"

/// @brief Load configuration and store it from a CAPIO-CL TOML configuration file
class capiocl::configuration::CapioClConfiguration {
    friend class engine::Engine;
    std::unordered_map<std::string, std::string> config;

  protected:
    void set(const std::string &key, std::string value);

  public:
    explicit CapioClConfiguration();
    ~CapioClConfiguration() = default;

    void load(const std::filesystem::path &path);
    void getParameter(const std::string &key, int *value) const;
    void getParameter(const std::string &key, std::string *value) const;
};

#endif