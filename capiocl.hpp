#ifndef CAPIO_CL_CAPIOCL_HPP
#define CAPIO_CL_CAPIOCL_HPP

#include <climits>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <jsoncons/basic_json.hpp>
#include <jsoncons_ext/jsonschema/jsonschema.hpp>
#include <string>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 1024
#endif

/// @brief Namespace containing all the CAPIO-CL related code
namespace capiocl {

/// @brief Default workflow name for CAPIO-CL
constexpr char CAPIO_CL_DEFAULT_WF_NAME[] = "CAPIO_CL";

/// @brief CLI print constant
constexpr char CLI_LEVEL_INFO[]    = "[\033[1;32mCAPIO-CL\033[0m";
/// @brief CLI print constant
constexpr char CLI_LEVEL_WARNING[] = "[\033[1;33mCAPIO-CL\033[0m";
/// @brief CLI print constant
constexpr char CLI_LEVEL_ERROR[]   = "[\033[1;31mCAPIO-CL\033[0m";
/// @brief CLI print constant
constexpr char CLI_LEVEL_JSON[]    = "[\033[1;34mCAPIO-CL\033[0m";

/// @brief Namespace for CAPIO-CL Firing Rules
namespace fire_rules {
/// @brief FnU Streaming Rule
constexpr char NO_UPDATE[] = "no_update";
/// @brief FoC Streaming Rule
constexpr char UPDATE[]    = "update";

/**
 * Sanitize fire rule from input
 * @param input
 * @return sanitized fire rule
 */
inline std::string sanitize_fire_rule(const std::string &input) {
    if (input == NO_UPDATE) {
        return NO_UPDATE;
    } else if (input == UPDATE) {
        return UPDATE;
    } else {
        throw std::invalid_argument("Input commit rule is not a vlid CAPIO-CL rule");
    }
}
} // namespace fire_rules

/// @brief Namespace for CAPIO-CL Commit Rules
namespace commit_rules {
/// @brief CoC Streaming Rule
constexpr char ON_CLOSE[]       = "on_close";
/// @brief CoF Streaming Rule
constexpr char ON_FILE[]        = "on_file";
/// @brief CnF Streaming Rule
constexpr char ON_N_FILES[]     = "on_n_files";
/// @brief CoT Streaming Rule
constexpr char ON_TERMINATION[] = "on_termination";

/**
 * Sanitize commit rule from input
 * @param input
 * @return sanitized commit rule
 */
inline std::string sanitize_commit_rule(const std::string &input) {
    if (input == ON_CLOSE) {
        return ON_CLOSE;
    } else if (input == ON_FILE) {
        return ON_FILE;
    } else if (input == ON_N_FILES) {
        return ON_N_FILES;
    } else if (input == ON_TERMINATION) {
        return ON_TERMINATION;
    } else {
        throw std::invalid_argument("Input commit rule is not a vlid CAPIO-CL rule");
    }
}
} // namespace commit_rules

/// @brief Available versions of CAPIO-CL language
struct CAPIO_CL_VERSION final {
    /// @brief Release 1.0 of CAPIO-CL
    static constexpr char V1[] = "1.0";
};

/**
 * Print a message to standard out. Used to log messages related to the CAPIO-CL #Engine
 * @param message_type Type of message to print.
 * @param message_line
 */
inline void print_message(const std::string &message_type = "",
                          const std::string &message_line = "") {
    static std::string *node_name = nullptr;
    if (node_name == nullptr) {
        node_name = new std::string(HOST_NAME_MAX, ' '); // LCOV_EXCL_LINE
        gethostname(node_name->data(), HOST_NAME_MAX);
    }
    if (message_type.empty()) {
        std::cout << std::endl;
    } else {
        std::cout << message_type << " " << node_name->c_str() << "] " << message_line << std::endl
                  << std::flush;
    }
}

/**
 * @brief Custom exception thrown when parsing a CAPIO-CL configuration file by #Parser
 */
class MonitorException final : public std::exception {
    std::string message;

  public:
    /**
     * @brief Construct a new CAPIO-CL Exception
     * @param msg Error Message that raised this exception
     */
    explicit MonitorException(const std::string &msg) : message(msg) {
        print_message(CLI_LEVEL_ERROR, msg);
    }

    /**
     * Get the description of the error causing the exception
     * @return
     */
    [[nodiscard]] const char *what() const noexcept override { return message.c_str(); }
};

/**
 * Class to monitor runtime dependent information on CAPIO-CL related paths, such as commitment
 * status and Home Node Policies
 */
class Monitor {
    friend class Engine;

    bool *continue_execution;

    std::thread *commit_listener_thread;

    mutable std::mutex committed_lock;
    mutable std::vector<std::string> _committed_files;

    std::string MULTICAST_ADDR;
    int MULTICAST_PORT;

    typedef enum { COMMIT = '!', REQUEST = '?' } MESSAGE_COMMANDS;

    /**
     * Thread function to monitor the occurrence of commitment of CAPIO-CL files
     *
     * @param committed_files Reference to std::vector containing committed files
     * @param lock Lock to acquire mutex access to committed_files
     * @param continue_execution Reference to boolean variable to know when to terminate
     * @param ip_addr Socket multicast IP Address
     * @param ip_port Socket multicast IP Port
     */
    static void commit_listener(std::vector<std::string> &committed_files, std::mutex &lock,
                                const bool *continue_execution, const std::string &ip_addr,
                                int ip_port);

    /**
     * Inner static member to send the commit message of a file
     * @param ip_addr  Multicast IP addr to send data
     * @param ip_port Multicast IP port to send data
     * @param path File to be sent as committed
     * @param action ! to send commit status, ? to request status
     */
    static void _send_message(const std::string &ip_addr, int ip_port, const std::string &path,
                              MESSAGE_COMMANDS action = COMMIT);

  protected:
    /**
     * Check whether a file is committed or not. First look into _committed_files. If not found
     * then look into the file system for a committed token. If the committed token is not found
     * then return false.
     *
     * TODO: commit token on FS
     *
     * @param path path to check for the commit status
     * @return
     */
    bool isCommitted(const std::filesystem::path &path) const;

    /**
     * Set a file to be committed. First send a multicast message, and then generate a
     * commit token
     *
     * TODO: commit token on FS
     * @param path Path of file to commit
     */
    void setCommitted(const std::filesystem::path &path) const;

  public:
    /**
     * Default constructor
     *
     * @param ip_addr Multicast Group IP Address
     * @param ip_port Multicast Group IP Port
     * @throws MonitorException
     */
    explicit Monitor(const std::string &ip_addr, int ip_port);
    ~Monitor();
};

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
    friend class Serializer;
    bool store_all_in_memory = false;

    /// @brief Monitor instance to check runtime information of CAPIO-CL files
    Monitor *monitor;

    /// @brief Node name variable used to handle home node policies
    std::string node_name;

    /// @brief Name of the current workflow name
    std::string workflow_name;

    // LCOV_EXCL_START
    /// @brief Internal CAPIO-CL #Engine storage entity. Each CapioCLEntry is an entry for a given
    /// file handled by CAPIO-CL
    struct CapioCLEntry final {
        std::vector<std::string> producers;
        std::vector<std::string> consumers;
        std::vector<std::filesystem::path> file_dependencies;
        std::string commit_rule            = commit_rules::ON_TERMINATION;
        std::string fire_rule              = fire_rules::UPDATE;
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
    explicit Engine() {
        node_name = std::string(1024, '\0');
        gethostname(node_name.data(), node_name.size());
        node_name.resize(std::strlen(node_name.c_str()));

        if (const char *_wf_name = std::getenv("WORKFLOW_NAME"); _wf_name != nullptr) {
            this->workflow_name = _wf_name;
        } else {
            this->workflow_name = CAPIO_CL_DEFAULT_WF_NAME;
        }

        monitor = new Monitor("224.224.224.1", 12345);
    }

    /// @brief Default destructor
    ~Engine() { delete monitor; }

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
    std::string getHomeNode(const std::filesystem::path &path) const;

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
     * @brief Check for equality between two instances of #Engine
     * @param other reference to another #Engine class instance
     * @return true if both this instance and other are equivalent. false otherwise.
     */
    bool operator==(const Engine &other) const;
};

/**
 * @brief Custom exception thrown when parsing a CAPIO-CL configuration file by #Parser
 */
class ParserException final : public std::exception {
    std::string message;

  public:
    /**
     * @brief Construct a new CAPIO-CL Exception
     * @param msg Error Message that raised this exception
     */
    explicit ParserException(const std::string &msg) : message(msg) {
        print_message(CLI_LEVEL_ERROR, msg);
    }

    /**
     * Get the description of the error causing the exception
     * @return
     */
    [[nodiscard]] const char *what() const noexcept override { return message.c_str(); }
};

/// @brief Contains the code to parse a JSON based CAPIO-CL configuration file
class Parser final {

    /// @brief Available parsers for CAPIO-CL
    struct available_parsers {
        /**
         * Parser for the V1 Specification of the CAPIO-CL language
         * @param source Path of CAPIO-CL configuration file
         * @param resolve_prefix Prefix to prepend to path if found to be relative
         * @param store_only_in_memory Flag to set to returned instance of #Engine if required to
         * store all files in memory
         * @return Parsed Engine.
         */
        static Engine *parse_v1(const std::filesystem::path &source,
                                const std::filesystem::path &resolve_prefix,
                                bool store_only_in_memory);
    };

    /**
     * Load a json Schema into memory from a byte encoded array castable to a const char[]
     * @param data Array of byte encoded json Schema
     * @return The generated
     */
    static jsoncons::jsonschema::json_schema<jsoncons::json> loadSchema(const char *data);

  protected:
    /**
     * Resolve (if relative) a path to an absolute one using the provided prefix
     * @param path Path to resolve
     * @param prefix Prefix
     * @return Absolute path constructed from path
     */
    static std::filesystem::path resolve(std::filesystem::path path,
                                         const std::filesystem::path &prefix);

  public:
    /**
     * Validate a CAPIO-CL configuration file according to the JSON schema of the language
     * @param doc The loaded CAPIO-CL configuration file
     */
    static void validate_json(const jsoncons::json &doc);

    /**
     * @brief Perform the parsing of the capio_server configuration file
     *
     * @param source Input CAPIO-CL Json configuration File
     * @param resolve_prefix If paths are found to be relative, they are appended to this path
     * @param store_only_in_memory Set to true to set all files to be stored in memory
     * @return #Engine instance with the information provided by  the config file
     * @throw ParserException
     */
    static Engine *parse(const std::filesystem::path &source,
                         const std::filesystem::path &resolve_prefix = "",
                         bool store_only_in_memory                   = false);
};

/**
 * @brief Custom exception thrown when serializing an instance of #Engine
 */
class SerializerException final : public std::exception {
    std::string message;

  public:
    /**
     * @brief Construct a new CAPIO-CL Exception
     * @param msg Error Message that raised this exception
     */
    explicit SerializerException(const std::string &msg) : message(msg) {
        print_message(CLI_LEVEL_ERROR, msg);
    }

    /**
     * Get the description of the error causing the exception
     * @return
     */
    [[nodiscard]] const char *what() const noexcept override { return message.c_str(); }
};

/// @brief Dump the current loaded CAPIO-CL configuration from class #Engine to a CAPIO-CL
/// configuration file.
class Serializer final {

    /// @brief Available serializers for CAPIO-CL
    struct available_serializers {
        /**
         * @brief Dump the current configuration loaded into an instance of  #Engine to a CAPIO-CL
         * VERSION 1 configuration file.
         *
         * @param engine instance of Engine to dump
         * @param filename path of output file
         * @throws SerializerException
         */
        static void serialize_v1(const Engine &engine, const std::filesystem::path &filename);
    };

  public:
    /**
     * @brief Dump the current configuration loaded into an instance of  #Engine to a CAPIO-CL
     * configuration file.
     *
     * @param engine instance of Engine to dump
     * @param filename path of output file
     * @param version Version of CAPIO-CL used to generate configuration files.
     */
    static void dump(const Engine &engine, const std::filesystem::path &filename,
                     const std::string &version = CAPIO_CL_VERSION::V1);
};
} // namespace capiocl

#endif // CAPIO_CL_CAPIOCL_HPP
