#ifndef CAPIO_CL_CONFIGURATION_H
#define CAPIO_CL_CONFIGURATION_H
#include <filesystem>
#include <unordered_map>

#include "capiocl.hpp"

/// @brief Struct containing key value pairs for capio-cl runtime configuration
typedef struct {
    /// @brief Key of option
    std::string k;
    /// @brief Value of option
    std::string v;
} ConfigurationEntry;

/// @brief Defaults values keys for runtime options of CAPIO-CL
struct capiocl::configuration::defaults {
    /// @brief Multicast monitor commit IP
    static ConfigurationEntry DEFAULT_MONITOR_MCAST_IP;
    /// @brief Multicast monitor commit PORT
    static ConfigurationEntry DEFAULT_MONITOR_MCAST_PORT;
    /// @brief Multicast monitor delay before following operation
    static ConfigurationEntry DEFAULT_MONITOR_MCAST_DELAY;
    /// @brief Multicast monitor homenode IP
    static ConfigurationEntry DEFAULT_MONITOR_HOMENODE_IP;
    /// @brief Multicast monitor homenode PORT
    static ConfigurationEntry DEFAULT_MONITOR_HOMENODE_PORT;
};

/// @brief Load configuration and store it from a CAPIO-CL TOML configuration file
class capiocl::configuration::CapioClConfiguration {
    friend class engine::Engine;
    std::unordered_map<std::string, std::string> config;

  protected:
    /**
     * Set a key value pair explicitly
     * @param key Option key name
     * @param value Value
     */
    void set(const std::string &key, std::string value);

    /**
     * Set a capio-cl configuration option through a ConfigurationEntry object
     * @param entry
     */
    void set(const ConfigurationEntry &entry);

  public:
    explicit CapioClConfiguration();
    ~CapioClConfiguration() = default;

    /**
     * Load a configuiration from a TOML file
     * @param path
     */
    void load(const std::filesystem::path &path);

    /**
     * Get a string value
     * @param key key of option to get
     * @param value reference in which value will be stored
     */
    void getParameter(const std::string &key, int *value) const;

    /**
     * Get a integer value
     * @param key key of option to get
     * @param value reference in which value will be stored
     */
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