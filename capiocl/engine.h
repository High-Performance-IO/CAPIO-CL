#ifndef CAPIO_CL_ENGINE_H
#define CAPIO_CL_ENGINE_H
#include <vector>

#include "capiocl.hpp"
#include "capiocl/monitor.h"
#include "capiocl/serializer.h"
#include "capiocl/webapi.h"

/// @brief Namespace containing the CAPIO-CL Engine
namespace capiocl::engine {
/**
 * @brief Engine for managing CAPIO-CL configuration entries.
 * The CapioCLEngine class stores and manages configuration rules for files
 * and directories as defined in the CAPIO-CL configuration file.
 * It maintains producers, consumers, commit rules, fire rules, and other
 * metadata associated with files or directories.
 * Each entry in the configuration associates a path with:
 * - Producers and consumers
 * - Commit and fire rules
 * - Flags such as permanent, excluded, directory/file type
 * - Commit-on-close counters and expected directory file counts
 * - File dependencies
 * - Regex matchers for globbing
 * - Storage policy (in-memory or on filesystem)
 */
class Engine final {
    friend class serializer::Serializer;
    bool store_all_in_memory = false;

    ///@brief Configuration imported from CAPIO-CL config TOML file
    configuration::CapioClConfiguration configuration;

    /// @brief Monitor instance to check runtime information of CAPIO-CL files
    monitor::Monitor monitor;

    /// @brief Node name variable used to handle home node policies
    std::string node_name;

    /// @brief Name of the current workflow name
    std::string workflow_name;

    /// @brief CAPIO-CL APIs Web Server
    std::unique_ptr<webapi::CapioClWebApiServer> webapi_server;

    // LCOV_EXCL_START
    /// @brief Internal CAPIO-CL Engine storage entity. Each CapioCLEntry is an entry for a given
    /// file handled by CAPIO-CL
    struct CapioCLEntry final {
        std::vector<std::string> producers;
        std::vector<std::string> consumers;
        std::vector<std::filesystem::path> file_dependencies;
        std::string commit_rule            = commitRules::ON_TERMINATION;
        std::string fire_rule              = fireRules::UPDATE;
        long directory_children_count      = 0;
        long commit_on_close_count         = 0;
        bool enable_directory_count_update = true; // whether to update or not directory item count
        bool store_in_memory               = false;
        bool permanent                     = false;
        bool excluded                      = false;
        bool is_file                       = true;
    };

    // LCOV_EXCL_STOP

    /// @brief Hash map used to store the configuration from CAPIO-CL
    mutable std::unordered_map<std::string, CapioCLEntry> _capio_cl_entries;

    /**
     * @brief Utility method to truncate a string to its last @p n characters. This is only used
     * within the print method
     *
     * If the string is longer than @p n, it prefixes the result with "[..]".
     *
     * @param str Input string.
     * @param n Number of characters to keep from the end.
     * @return Truncated string with "[..]" prefix.
     */
    static std::string truncateLastN(const std::string &str, const std::size_t n) {
        if (str.length() > n) {
            return "[..] " + str.substr(str.length() - n);
        }
        return str;
    }

    /**
     * @brief Insert a new empty default file in #_capio_cl_entries
     * @param path File path name
     */
    void _newFile(const std::filesystem::path &path) const;

    /**
     * @brief Updates the number of entries in the parent directory of the given path.
     *
     * @note The computed value remains valid only until `setDirectoryFileCount()` is called.
     * Once `setDirectoryFileCount()` is used, this method is no longer responsible for computing
     * the number of files within a directory. This is because, when the user explicitly sets
     * the expected number of files in a directory, it becomes impossible to determine whether
     * that provided count includes or excludes the files automatically computed from CAPIO-CL
     * information, or if it includes also all future created CAPIO-CL file entries or not.
     *
     * @param path The path whose parent directory entry count should be updated.
     */
    void compute_directory_entry_count(const std::filesystem::path &path) const;

  public:
    /// @brief Class constructor
    explicit Engine(bool use_default_settings = true);

    /// @brief Print the current CAPIO-CL configuration.
    void print() const;

    /**
     * @brief Check whether a file is contained in the configuration.
     * The lookup is performed by exact match or by regex globbing.
     *
     * @param file Path of the file to check.
     * @return true if the file is contained, false otherwise.
     */
    bool contains(const std::filesystem::path &file) const;

    /// @brief return the number of entries in the current configuration
    size_t size() const;

    /**
     * @brief Add a new CAPIO-CL configuration entry.
     *
     * @param path Path of the file or directory.
     * @param producers List of producer applications.
     * @param consumers List of consumer applications.
     * @param commit_rule Commit rule to apply.
     * @param fire_rule Fire rule to apply.
     * @param permanent Whether the file/directory is permanent.
     * @param exclude Whether the file/directory is excluded.
     * @param dependencies List of dependent files.
     */
    void add(std::filesystem::path &path, std::vector<std::string> &producers,
             std::vector<std::string> &consumers, const std::string &commit_rule,
             const std::string &fire_rule, bool permanent, bool exclude,
             std::vector<std::filesystem::path> &dependencies);

    /**
     * @brief Add a new producer to a file entry.
     *
     * @param path File path.
     * @param producer Application name of the producer.
     */
    void addProducer(const std::filesystem::path &path, std::string &producer);

    /**
     * @brief Add a new consumer to a file entry.
     *
     * @param path File path.
     * @param consumer Application name of the consumer.
     */
    void addConsumer(const std::filesystem::path &path, std::string &consumer);

    /**
     * @brief Add a new file dependency, when rule is commit_on_file
     *As a side effect, the file identified by path, has the commit rule set to Commit on Files
     *
     * @param path targeted file path
     * @param file_dependency the new file for this the path is subject to commit rule
     */
    void addFileDependency(const std::filesystem::path &path,
                           std::filesystem::path &file_dependency);

    /**
     * @brief Create a new CAPIO file entry. Commit and fire rules are automatically computed using
     * the longest prefix match from the configuration.
     *
     * @param path Path of the new file.
     */
    void newFile(const std::filesystem::path &path);

    /**
     * @brief Remove a file from the configuration.
     * @param path Path of the file to remove.
     */
    void remove(const std::filesystem::path &path) const;

    /**
     * @brief Set the commit rule of a file.
     * @param path File path.
     * @param commit_rule Commit rule string.
     * @throw std::invalid_argument if commit rule is not a valid CAPIO-CL commit rule
     */
    void setCommitRule(const std::filesystem::path &path, const std::string &commit_rule);

    /**
     * @brief Set the fire rule of a file.
     * @param path File path.
     * @param fire_rule Fire rule string.
     * @throw std::invalid_argument if fire rule is not a valid CAPIO-CL Firing rule
     */
    void setFireRule(const std::filesystem::path &path, const std::string &fire_rule);

    /**
     * @brief Mark a file as permanent or not.
     * @param path File path.
     * @param value true to mark permanent, false otherwise.
     */
    void setPermanent(const std::filesystem::path &path, bool value);

    /**
     * @brief Mark a file as excluded or not.
     * @param path File path.
     * @param value true to exclude, false otherwise.
     */
    void setExclude(const std::filesystem::path &path, bool value);

    /**
     * @brief Mark a path as a directory.
     * @param path Path to mark.
     */
    void setDirectory(const std::filesystem::path &path);

    /**
     * @brief Mark a path as a file.
     * @param path Path to mark.
     */
    void setFile(const std::filesystem::path &path);

    /**
     * @brief Set the commit-on-close counter. The file will be committed after @p num close
     * operations.
     * @param path File path.
     * @param num Number of close operations before commit.
     */
    void setCommitedCloseNumber(const std::filesystem::path &path, long num);

    /**
     * @brief Sets the expected number of files in a directory.
     *
     * @note When using this method, `capiocl::Engine` will no longer automatically compute
     * the number of files contained within the directory specified by @p path. This is because
     * there is no way to determine whether the user-provided count includes or excludes the files
     * automatically detected by CAPIO-CL. Also, there is no way to know whether the provided number
     * is already inclusive of the possible future generated children files.
     *
     * @param path The directory path.
     * @param num The expected number of files in the directory.
     */

    void setDirectoryFileCount(const std::filesystem::path &path, long num);

    /**
     * @brief Set the dependencies of a file. This method as a side effect sets the commit rule to
     * Commit on Files.
     * @param path File path.
     * @param dependencies List of dependent files.
     */
    void setFileDeps(const std::filesystem::path &path,
                     const std::vector<std::filesystem::path> &dependencies);

    /**
     * @brief Store the file in memory only.
     * @param path File path.
     */
    void setStoreFileInMemory(const std::filesystem::path &path);

    /// @brief set all files to be stored in memory. Once this method is called, all new files will
    ///        be stored in memory unless afterward an explicit call to setStoreFileInFileSystem()
    ///        is issued targeting the newly created file
    void setAllStoreInMemory();

    /**
     * @brief Store the file on the file system.
     * @param path File path.
     */
    void setStoreFileInFileSystem(const std::filesystem::path &path);

    /**
     * Set current orkflow name
     * @param name Name of the workflow
     */
    void setWorkflowName(const std::string &name);

    /**
     * @brief Get the expected number of files in a directory.
     * @param path Directory path.
     * @return Expected file count.
     */
    long getDirectoryFileCount(const std::filesystem::path &path) const;

    /// @brief Get the commit rule of a file.
    std::string getCommitRule(const std::filesystem::path &path) const;

    /// @brief Get the fire rule of a file.
    std::string getFireRule(const std::filesystem::path &path) const;

    /// @brief Get the producers of a file.
    std::vector<std::string> getProducers(const std::filesystem::path &path) const;

    /// @brief Get the consumers of a file.
    std::vector<std::string> getConsumers(const std::filesystem::path &path) const;

    /// @brief Get the commit-on-close counter for a file.
    long getCommitCloseCount(const std::filesystem::path::iterator::reference &path) const;

    /// @brief Get file dependencies.
    std::vector<std::filesystem::path>
    getCommitOnFileDependencies(const std::filesystem::path &path) const;

    /// @brief Get the list of files stored in memory.
    std::vector<std::string> getFileToStoreInMemory() const;

    /// @brief Get the home node of a file.
    std::set<std::string> getHomeNode(const std::filesystem::path &path) const;

    /// @brief Set the home node for a given path
    void setHomeNode(const std::filesystem::path &path) const;

    /// @brief Get current workflow name loaded from memory
    const std::string &getWorkflowName() const;

    /**
     * @brief Check if a process is a producer for a file.
     * @param path File path.
     * @param app_name Application name.
     * @return true if the process is a producer, false otherwise.
     */
    bool isProducer(const std::filesystem::path &path, const std::string &app_name) const;

    /**
     * @brief Check if a process is a consumer for a file.
     * @param path File path.
     * @param app_name Application name.
     * @return true if the process is a consumer, false otherwise.
     */
    bool isConsumer(const std::filesystem::path &path, const std::string &app_name) const;

    /**
     * @brief Check if a file is firable, that is fire rule is no_update.
     * @param path File path.
     * @return true if the file is firable, false otherwise.
     */
    bool isFirable(const std::filesystem::path &path) const;

    /**
     * @brief Check if a path refers to a file.
     * @param path File path.
     * @return true if the path is a file, false otherwise.
     */
    bool isFile(const std::filesystem::path &path) const;

    /**
     * @brief Check if a path is excluded.
     * @param path File path.
     * @return true if excluded, false otherwise.
     */
    bool isExcluded(const std::filesystem::path &path) const;

    /**
     * @brief Check if a path is a directory.
     * @param path Directory path.
     * @return true if directory, false otherwise.
     */
    bool isDirectory(const std::filesystem::path &path) const;

    /**
     * @brief Check if a file is stored in memory.
     * @param path File path to query
     * @return true if stored in memory, false otherwise.
     */
    bool isStoredInMemory(const std::filesystem::path &path) const;

    /**
     * @brief Check if file should remain on file system after workflow terminates
     * @param path File path to query
     * @return True if file should persist on storage after workflow termination.
     */
    bool isPermanent(const std::filesystem::path &path) const;

    /**
     * Check whether the path is committed or not
     * @param path
     * @return
     */
    bool isCommitted(const std::filesystem::path &path) const;

    /**
     * Set file indicated by path as committed
     * @param path
     */
    void setCommitted(const std::filesystem::path &path) const;

    /**
     * Get all the paths that are presents within the Current instance of Engine
     * @return
     */
    std::vector<std::string> getPaths() const;

    /**
     * @brief Check for equality between two instances of Engine
     * @param other reference to another Engine class instance
     * @return true if both this instance and other are equivalent. false otherwise.
     */
    bool operator==(const Engine &other) const;

    /**
     * Load a CAPIO-CL TOML configuration file
     * @param path
     */
    void loadConfiguration(const std::string &path);

    /**
     * Use default CAPIO-CL TOML configuration.
     */
    void useDefaultConfiguration();

    /**
     * Start the thread involved in the handling of dynamic changes to CapioCl configuration
     * @param address address to listen to. defaulto to 127.0.0.1
     * @param port Port to listen to. defaults to 5520
     */
    void startApiServer(const std::string &address = "127.0.0.1", int port = 5520);
};

} // namespace capiocl::engine

#endif // CAPIO_CL_ENGINE_H