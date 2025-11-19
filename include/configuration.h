#ifndef CAPIO_CL_CONFIGURATION_H
#define CAPIO_CL_CONFIGURATION_H
#include <filesystem>
#include <unordered_map>

#include "capiocl.hpp"

typedef struct {
    std::string k;
    std::string v;
} ConfigurationEntry;

struct capiocl::configuration::defaults {
    static ConfigurationEntry DEFAULT_MONITOR_MCAST_IP;
    static ConfigurationEntry DEFAULT_MONITOR_MCAST_PORT;
    static ConfigurationEntry DEFAULT_MONITOR_MCAST_DELAY;
    static ConfigurationEntry DEFAULT_MONITOR_HOMENODE_IP;
    static ConfigurationEntry DEFAULT_MONITOR_HOMENODE_PORT;
};

/// @brief Load configuration and store it from a CAPIO-CL TOML configuration file
class capiocl::configuration::CapioClConfiguration {
    friend class engine::Engine;
    std::unordered_map<std::string, std::string> config;

  protected:
    void set(const std::string &key, std::string value);
    void set(const ConfigurationEntry &entry);

  public:
    explicit CapioClConfiguration();
    ~CapioClConfiguration() = default;

    void load(const std::filesystem::path &path);
    void getParameter(const std::string &key, int *value) const;
    void getParameter(const std::string &key, std::string *value) const;
};

/**
 * @brief Custom exception thrown when handling a CAPIO-CL TOML configuration file
 */
class capiocl::configuration::CapioClConfigurationException final : public std::exception {
    std::string message;

  public:
    /**
     * @brief Construct a new CAPIO-CL Exception
     * @param msg Error Message that raised this exception
     */
    explicit CapioClConfigurationException(const std::string &msg);

    /**
     * Get the description of the error causing the exception
     * @return
     */
    [[nodiscard]] const char *what() const noexcept override { return message.c_str(); }
};

#endif