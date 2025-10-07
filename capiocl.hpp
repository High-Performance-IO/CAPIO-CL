#ifndef CAPIO_CL_CAPIOCL_HPP
#define CAPIO_CL_CAPIOCL_HPP

#include <climits>
#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <unistd.h>
#endif

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 1024
#endif

/**
 * Compatibility layer for CAPIO logger.
 */
#ifndef CAPIO_LOG
#define START_LOG(...)
#define LOG(...)
#define DBG(...)
#define ERR_EXIT(...) exit(EXIT_FAILURE);
#endif

namespace capiocl {

constexpr char CAPIO_CL_DEFAULT_WF_NAME[] = "CAPIO_CL";

constexpr char CLI_LEVEL_INFO[]    = "[\033[1;32mCAPIO-CL\033[0m";
constexpr char CLI_LEVEL_WARNING[] = "[\033[1;33mCAPIO-CL\033[0m";
constexpr char CLI_LEVEL_ERROR[]   = "[\033[1;31mCAPIO-CL\033[0m";
constexpr char CLI_LEVEL_JSON[]    = "[\033[1;34mCAPIO-CL\033[0m";

// CAPIO streaming semantics
constexpr char MODE_NO_UPDATE[]           = "no_update";
constexpr char MODE_UPDATE[]              = "update";
constexpr char COMMITTED_ON_CLOSE[]       = "on_close";
constexpr char COMMITTED_ON_FILE[]        = "on_file";
constexpr char COMMITTED_N_FILES[]        = "n_files";
constexpr char COMMITTED_ON_TERMINATION[] = "on_termination";

/**
 * Print a message to standard out. Used to log messages related to the CAPIO-CL engine
 * @param message_type Type of message to print.
 * @param message_line
 */
inline void print_message(const std::string &message_type = "",
                          const std::string &message_line = "") {
    static std::string node_name;
    if (node_name.empty()) {
        node_name.reserve(HOST_NAME_MAX);
        gethostname(node_name.data(), HOST_NAME_MAX);
    }
    if (message_type.empty()) {
        std::cout << std::endl;
    } else {
        std::cout << message_type << " " << node_name.c_str() << "] " << message_line << std::endl
                  << std::flush;
    }
}

/**
 * @brief Engine for managing CAPIO-CL configuration entries.
 *
 * The CapioCLEngine class stores and manages configuration rules for files
 * and directories as defined in the CAPIO-CL configuration file.
 * It maintains producers, consumers, commit rules, fire rules, and other
 * metadata associated with files or directories.
 *
 * Each entry in the configuration associates a path with:
 * - Producers and consumers
 * - Commit and fire rules
 * - Flags such as permanent, excluded, directory/file type
 * - Commit-on-close counters and expected directory file counts
 * - File dependencies
 * - Regex matchers for globbing
 * - Storage policy (in-memory or on filesystem)
 */
class Engine {

    std::string node_name;

    /**
     * Hash map used to store the configuration from CAPIO-CL
     */
    std::unordered_map<std::filesystem::path,               ///< Path name
                       std::tuple<std::vector<std::string>, ///< Producers list                 [0]
                                  std::vector<std::string>, ///< Consumers list                 [1]
                                  std::string,              ///< Commit rule                    [2]
                                  std::string,              ///< Fire rule                      [3]
                                  bool,                     ///< Permanent flag                 [4]
                                  bool,                     ///< Excluded flag                  [5]
                                  bool,                     ///< Is file (false = directory)    [6]
                                  long,                     ///< Commit-on-close count          [7]
                                  long,                     ///< Expected directory file count  [8]
                                  std::vector<std::filesystem::path>, ///< File dependencies    [9]
                                  bool>>                    ///< Store in FS (false = memory)   [10]
        _locations;

    /**
     * @brief Utility method to truncate a string to its last @p n characters. This is only used
     * within the print method
     *
     * If the string is longer than @p n, it prefixes the result with "[..] ".
     *
     * @param str Input string.
     * @param n Number of characters to keep from the end.
     * @return Truncated string with optional "[..]" prefix.
     */
    static std::string truncateLastN(const std::string &str, const std::size_t n) {
        return str.length() > n ? "[..] " + str.substr(str.length() - n) : str;
    }

  protected:
    /**
     * @brief Access the internal location map.
     *
     * @return Pointer to the internal location mapping.
     */
    const auto *getLocations() const { return &_locations; }

  public:
    /**
     * Class constructor
     */
    explicit Engine() {
        node_name.reserve(HOST_NAME_MAX);
        gethostname(node_name.data(), HOST_NAME_MAX);
        print_message(CLI_LEVEL_INFO, "Instance created");
    }

    /**
     * @brief Print the current CAPIO-CL configuration.
     */
    void print() const;

    /**
     * @brief Check whether a file is contained in the configuration.
     *
     * The lookup is performed by exact match or by regex globbing.
     *
     * @param file Path of the file to check.
     * @return true if the file is contained, false otherwise.
     */
    bool contains(const std::filesystem::path &file);

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
             const std::vector<std::filesystem::path> &dependencies);

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
     *
     * @param path
     * @param file_dependency
     */
    void addFileDependency(const std::filesystem::path &path, std::string &file_dependency);

    /**
     * @brief Create a new CAPIO file entry.
     *
     * Commit and fire rules are automatically computed using the
     * longest prefix match from the configuration.
     *
     * @param path Path of the new file.
     */
    void newFile(const std::filesystem::path &path);

    /**
     * @brief Remove a file from the configuration.
     *
     * @param path Path of the file to remove.
     */
    void remove(const std::filesystem::path &path);

    /**
     * @brief Set the commit rule of a file.
     *
     * @param path File path.
     * @param commit_rule Commit rule string.
     */
    void setCommitRule(const std::filesystem::path &path, const std::string &commit_rule);

    /**
     * @brief Set the fire rule of a file.
     *
     * @param path File path.
     * @param fire_rule Fire rule string.
     */
    void setFireRule(const std::filesystem::path &path, const std::string &fire_rule);

    /**
     * @brief Mark a file as permanent or not.
     *
     * @param path File path.
     * @param value true to mark permanent, false otherwise.
     */
    void setPermanent(const std::filesystem::path &path, bool value);

    /**
     * @brief Mark a file as excluded or not.
     *
     * @param path File path.
     * @param value true to exclude, false otherwise.
     */
    void setExclude(const std::filesystem::path &path, bool value);

    /**
     * @brief Mark a path as a directory.
     *
     * @param path Path to mark.
     */
    void setDirectory(const std::filesystem::path &path);

    /**
     * @brief Mark a path as a file.
     *
     * @param path Path to mark.
     */
    void setFile(const std::filesystem::path &path);

    /**
     * @brief Set the commit-on-close counter.
     *
     * The file will be committed after @p num close operations.
     *
     * @param path File path.
     * @param num Number of close operations before commit.
     */
    void setCommitedCloseNumber(const std::filesystem::path &path, long num);

    /**
     * @brief Set the expected number of files in a directory.
     *
     * @param path Directory path.
     * @param num Expected file count.
     */
    void setDirectoryFileCount(const std::filesystem::path &path, long num);

    /**
     * @brief Set the dependencies of a file.
     *
     * Used for commit-on-file rules.
     *
     * @param path File path.
     * @param dependencies List of dependent files.
     */
    void setFileDeps(const std::filesystem::path &path,
                     const std::vector<std::filesystem::path> &dependencies);

    /**
     * @brief Store the file in memory only.
     *
     * @param path File path.
     */
    void setStoreFileInMemory(const std::filesystem::path &path);

    /// @brief set all files to be stored in memory
    void setAllStoreInMemory();

    /**
     * @brief Store the file on the file system.
     *
     * @param path File path.
     */
    void setStoreFileInFileSystem(const std::filesystem::path &path);

    /**
     * @brief Get the expected number of files in a directory.
     *
     * @param path Directory path.
     * @return Expected file count.
     */
    long getDirectoryFileCount(const std::filesystem::path &path);

    /// @brief Get the commit rule of a file.
    std::string getCommitRule(const std::filesystem::path &path);

    /// @brief Get the fire rule of a file.
    std::string getFireRule(const std::filesystem::path &path);

    /// @brief Get the producers of a file.
    std::vector<std::string> getProducers(const std::filesystem::path &path);

    /// @brief Get the consumers of a file.
    std::vector<std::string> getConsumers(const std::filesystem::path &path);

    /// @brief Get the commit-on-close counter for a file.
    long getCommitCloseCount(std::filesystem::path::iterator::reference path) const;

    /// @brief Get file dependencies.
    std::vector<std::filesystem::path>
    getCommitOnFileDependencies(const std::filesystem::path &path);

    /// @brief Get the list of files stored in memory.
    std::vector<std::filesystem::path> getFileToStoreInMemory();

    /// @brief Get the home node of a file.
    std::string getHomeNode(const std::filesystem::path &path);

    /**
     * @brief Check if a process is a producer for a file.
     *
     * @param path File path.
     * @param app_name Application name.
     * @return true if the process is a producer, false otherwise.
     */
    bool isProducer(const std::filesystem::path &path, const std::string &app_name);

    /**
     * @brief Check if a process is a consumer for a file.
     *
     * @param path File path.
     * @param app_name Application name.
     * @return true if the process is a consumer, false otherwise.
     */
    bool isConsumer(const std::filesystem::path &path, const std::string &app_name);

    /**
     * @brief Check if a file is firable.
     *
     * @param path File path.
     * @return true if the file is firable, false otherwise.
     */
    bool isFirable(const std::filesystem::path &path);

    /**
     * @brief Check if a path refers to a file.
     *
     * @param path File path.
     * @return true if the path is a file, false otherwise.
     */
    bool isFile(const std::filesystem::path &path);

    /**
     * @brief Check if a path is excluded.
     *
     * @param path File path.
     * @return true if excluded, false otherwise.
     */
    bool isExcluded(const std::filesystem::path &path) const;

    /**
     * @brief Check if a path is a directory.
     *
     * @param path Directory path.
     * @return true if directory, false otherwise.
     */
    bool isDirectory(const std::filesystem::path &path);

    /**
     * @brief Check if a file is stored in memory.
     *
     * @param path File path.
     * @return true if stored in memory, false otherwise.
     */
    bool isStoredInMemory(const std::filesystem::path &path);

    /**
     * @brief Check if file should remain on file system after workflow terminates
     *
     * @param path
     * @return
     */
    bool isPermanent(const std::filesystem::path &path);
};

/**
 * @brief Contains the code to parse a JSON based CAPIO-CL configuration file
 *
 */
class Parser {
    /**
     * @brief Check if a string is a representation of a integer number
     *
     * @param s
     * @return true
     * @return false
     */
    static bool isInteger(const std::string &s);

    /**
     * @brief compare two paths
     *
     * @param path
     * @param base
     * @return true if @p path is a subdirectory of base
     * @return false otherwise
     */
    static inline bool firstIsSubpathOfSecond(const std::filesystem::path &path,
                                              const std::filesystem::path &base);

  public:
    /**
     * @brief Perform the parsing of the capio_server configuration file
     *
     * @param source
     * @param resolve_prexix
     * @param store_only_in_memory Set to true to set all files to be stored in memory
     * @return Tuple with workflow name and CapioCLEngine instance with the information provided by
     * the config file
     */
    static std::tuple<std::string, Engine *> parse(const std::filesystem::path &source,
                                                   const std::filesystem::path &resolve_prexix = "",
                                                   bool store_only_in_memory = false);
};
} // namespace capiocl

#endif // CAPIO_CL_CAPIOCL_HPP
