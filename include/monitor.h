#ifndef CAPIO_CL_MONITOR_H
#define CAPIO_CL_MONITOR_H

#include <filesystem>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "configuration.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 1024
#endif

/// @brief Namespace containing the CAPIO-CL Monitor components
namespace capiocl::monitor {

/// @brief Constant value for when a home node is not found
static const std::string NO_HOME_NODE = "<NONE>";

/**
 * @brief Custom exception thrown when parsing a CAPIO-CL configuration file by Parser
 */
class MonitorException final : public std::exception {
    std::string message{};

  public:
    /**
     * @brief Construct a new CAPIO-CL Exception
     * @param msg Error Message that raised this exception
     */
    explicit MonitorException(const std::string &msg);

    /**
     * Get the description of the error causing the exception
     * @return
     */
    [[nodiscard]] const char *what() const noexcept override { return message.c_str(); }
};

/**
 * @brief Abstract interface for monitoring the commit state of files in CAPIO-CL.
 *
 * This class defines a common API for components that track whether files
 * have been marked as "committed" in the CAPIO-CL runtime. Implementations may
 * monitor commit state using different backends (e.g., filesystem signals or
 * multicast synchronization).
 *
 * The class is thread-safe through `committed_lock`, protecting `_committed_files`
 * which stores the list of committed file paths.
 */
class MonitorInterface {
  protected:
    /**
     * @brief Mutex protecting access to the committed file list.
     */
    mutable std::mutex committed_lock;

    /**
     * @brief Mutex protecting access to the home nodes list.
     */
    mutable std::mutex home_node_lock;

    /**
     * @brief List of committed file paths stored as strings.
     */
    mutable std::vector<std::string> _committed_files;

    /**
     * @brief Lookup table to get home node staring from path.
     * Key = file path;
     * Value = hostname of home node.
     */
    mutable std::unordered_map<std::string, std::string> _home_nodes;

    /**
     * @brief hostname of the current instance
     */
    mutable char _hostname[HOST_NAME_MAX] = {0};

  public:
    /**
     * @brief Virtual destructor for safe polymorphic deletion.
     */
    virtual ~MonitorInterface() = default;

    /**
     * @brief Check whether the given file has been committed.
     *
     * @param path Path to the file being queried.
     * @return true if the file is recorded as committed, false otherwise.
     */
    virtual bool isCommitted(const std::filesystem::path &path) const;

    /**
     * @brief Mark the given file as committed.
     *
     * @param path Path to the file to mark as committed.
     */
    virtual void setCommitted(const std::filesystem::path &path) const;

    /**
     * Set the current hostname as  the home node for a given path
     * @param path
     */
    virtual void setHomeNode(const std::filesystem::path &path) const;

    /**
     * Get the home node for a given path as reported by the implementation of this interface
     * @param path
     * @return the home node responsible for the given path
     */
    virtual const std::string &getHomeNode(const std::filesystem::path &path) const;
};

/**
 * @brief Monitor implementation that synchronizes file commit state via multicast messages.
 *
 * MulticastMonitor exchanges commit events between distributed processes using
 * multicast network messages. When one process commits a file, it broadcasts a message
 * so that other collaborators update their commit state.
 *
 * A background thread (`commit_listener_thread`) listens for notifications from
 * the network to update the internal commit list.
 */
class MulticastMonitor final : public MonitorInterface {

    static constexpr int MESSAGE_SIZE = (2 + PATH_MAX + PATH_MAX); ///< Max network message size.

    /**
     * @brief Background threads used to listen for commit messages and for home nodes.
     */
    std::thread commit_thread, home_node_thread;

    /**
     * @brief Multicast group IP address.
     */
    std::string MULTICAST_COMMIT_ADDR;

    std::string MULTICAST_HOME_NODE_ADDR;

    /**
     * @brief Multicast port number.
     */
    int MULTICAST_COMMIT_PORT{};

    int MULTICAST_HOME_NODE_PORT{};

    ///@brief Delay in milliseconds before checking again for a status change
    int MULTICAST_DELAY_MILLIS{};

    /**
     * @brief Supported network command types for commit messages.
     */
    typedef enum { SET = '!', GET = '?' } MESSAGE_COMMANDS;

    /**
     * @brief Send a commit or request message over multicast.
     *
     * @param ip_addr Destination multicast address.
     * @param ip_port Destination multicast port.
     * @param path File path associated with the message.
     * @param action The type of message to send (COMMIT or REQUEST).
     */
    static void _send_message(const std::string &ip_addr, int ip_port, const std::string &path,
                              MESSAGE_COMMANDS action);

    /**
     * @brief Background thread function to listen for commit messages.
     *
     * This function runs continuously while @p continue_execution remains true.
     * When commit events are received, the corresponding file paths are recorded
     * into @p committed_files.
     *
     * @param committed_files Vector storing committed file paths.
     * @param lock Mutex protecting shared access to committed_files.
     * @param ip_addr Multicast commit listen address.
     * @param ip_port Multicast commit listen port.
     */
    [[noreturn]] static void commit_listener(std::vector<std::string> &committed_files,
                                             std::mutex &lock, const std::string &ip_addr,
                                             int ip_port);

    /**
     * @brief Background thread function to listen for commit messages.
     *
     * This function runs continuously while @p continue_execution remains true.
     * When commit events are received, the corresponding file paths are recorded
     * into @p committed_files.
     *
     * @param home_nodes Vector storing committed file paths.
     * @param lock Mutex protecting shared access to committed_files.
     * @param ip_addr Multicast home node listen address.
     * @param ip_port Multicast home node listen port.
     */
    [[noreturn]] static void
    home_node_listener(std::unordered_map<std::string, std::string> &home_nodes, std::mutex &lock,
                       const std::string &ip_addr, int ip_port);

  public:
    /**
     * @brief Construct a multicast-based monitor.
     *
     *@param config const reference to CAPIO-CL configuration
     */
    MulticastMonitor(const capiocl::configuration::CapioClConfiguration &config);

    /**
     * @brief Destructor; stops listener thread and cleans resources.
     */
    ~MulticastMonitor() override;

    bool isCommitted(const std::filesystem::path &path) const override;
    void setCommitted(const std::filesystem::path &path) const override;
    void setHomeNode(const std::filesystem::path &path) const override;
    const std::string &getHomeNode(const std::filesystem::path &path) const override;
};

/**
 * @brief Monitor implementation that represents commit state using the filesystem.
 *
 * FileSystemMonitor uses token files on disk to record when a file is committed.
 * A committed file `<path>` is associated with a token file whose name is computed
 * from the path. Existence of the token file implies commit state.
 */
class FileSystemMonitor final : public MonitorInterface {

    typedef enum { COMMIT, HOME_NODE } CAPIO_CL_COMMIT_TOKEN_TYPES;

    /**
     * @brief Compute the token filename used to represent the commit state of the given file.
     *
     * @param path The original file path.
     * @param type Type of token to generate
     * @return A filesystem path representing the commit token.
     */
    static std::filesystem::path
    compute_capiocl_token_name(const std::filesystem::path &path,
                               CAPIO_CL_COMMIT_TOKEN_TYPES type = COMMIT);

    /**
     * @brief Create the commit token for a file.
     *
     * This creates an empty token file in the filesystem.
     *
     * @param path Path of the committed file.
     */
    static void generate_commit_token(const std::filesystem::path &path);

    /**
     * @brief Generate a token containing the home node for given path
     *
     * This creates a token file containing the homenode in the filesystem.
     *
     * @param path
     * @param home_node
     */
    static void generate_home_node_token(const std::filesystem::path &path,
                                         const std::string &home_node);

  public:
    /**
     * @brief Construct a filesystem-based commit monitor.
     */
    FileSystemMonitor();

    /**
     * @brief Destructor for FileSystemMonitor.
     */
    ~FileSystemMonitor() override = default;

    bool isCommitted(const std::filesystem::path &path) const override;
    void setCommitted(const std::filesystem::path &path) const override;
    void setHomeNode(const std::filesystem::path &path) const override;
    const std::string &getHomeNode(const std::filesystem::path &path) const override;
};

/**
 * @brief Class to monitor runtime dependent information on CAPIO-CL related paths, such as
 * commitment status and Home Node Policies
 */
class Monitor {
    friend class Engine;
    friend class MonitorInterface;

    std::vector<const MonitorInterface *> interfaces;

  public:
    /**
     * Check whether a file is committed or not. First look into _committed_files. If not found
     * then look into the file system for a committed token. If the committed token is not found
     * then return false.
     *
     * @param path path to check for the commit status
     * @return
     */
    [[nodiscard]] bool isCommitted(const std::filesystem::path &path) const;

    /**
     * Set a file to be committed. First send a multicast message, and then generate a
     * commit token
     *
     * @param path Path of file to commit
     */
    void setCommitted(std::filesystem::path path) const;

    /**
     * Add a new backend for monitor. Must be a derived class from MonitorInterface
     * @param interface
     */
    void registerMonitorBackend(const MonitorInterface *interface);

    /**
     * set the home node for given path to the current hostname, by calling the setHomeNode method
     * for all registered Monitor backends.
     * @param path
     */
    void setHomeNode(const std::filesystem::path &path) const;

    /**
     * Get set of home nodes from all registered backends
     * @param path
     * @return
     */
    [[nodiscard]] std::set<std::string> getHomeNode(const std::filesystem::path &path) const;

    ~Monitor();
};
} // namespace capiocl::monitor

#endif // CAPIO_CL_MONITOR_H