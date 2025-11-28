#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "capiocl.hpp"
#include "capiocl/monitor.h"
#include "capiocl/printer.h"

static std::tuple<int, sockaddr_in> outgoing_socket_multicast(const std::string &address,
                                                              const int port) {
    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = inet_addr(address.c_str());
    addr.sin_port        = htons(port);

    const int transmission_socket = socket(AF_INET, SOCK_DGRAM, 0);
    // LCOV_EXCL_START
    if (transmission_socket < 0) {
        throw capiocl::monitor::MonitorException(std::string("socket() failed: ") +
                                                 strerror(errno));
    }
    // LCOV_EXCL_STOP

    return {transmission_socket, addr};
}

static int incoming_socket_multicast(const std::string &address_ip, const int port,
                                     sockaddr_in &addr, socklen_t &addrlen) {
    constexpr int loopback   = 1; // enable reception of loopback messages
    constexpr int multi_bind = 1; // enable multiple sockets on same address

    addr                 = {};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = inet_addr(address_ip.c_str());
    addrlen              = sizeof(addr);

    ip_mreq mreq              = {};
    mreq.imr_multiaddr.s_addr = inet_addr(address_ip.c_str());
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    const int _socket = socket(AF_INET, SOCK_DGRAM, 0);

    // LCOV_EXCL_START
    if (_socket < 0) {
        throw capiocl::monitor::MonitorException(std::string("socket() failed: ") +
                                                 strerror(errno));
    }

    // Allow multiple sockets to bind to the same port
    if (setsockopt(_socket, SOL_SOCKET, SO_REUSEPORT, &multi_bind, sizeof(multi_bind)) < 0) {
        throw capiocl::monitor::MonitorException(std::string("REUSEPORT failed: ") +
                                                 strerror(errno));
    }

    // Bind to port
    if (bind(_socket, reinterpret_cast<sockaddr *>(&addr), addrlen) < 0) {
        throw capiocl::monitor::MonitorException(std::string("bind failed: ") + strerror(errno));
    }

    // Join multicast group
    if (setsockopt(_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        throw capiocl::monitor::MonitorException(std::string("join multicast failed: ") +
                                                 strerror(errno));
    }

    // Enable loopback
    if (setsockopt(_socket, IPPROTO_IP, IP_MULTICAST_LOOP, &loopback, sizeof(loopback)) < 0) {
        throw capiocl::monitor::MonitorException(std::string("loopback failed: ") +
                                                 strerror(errno));
    }
    // LCOV_EXCL_STOP

    return _socket;
}

[[noreturn]] void
capiocl::monitor::MulticastMonitor::commit_listener(std::vector<std::string> &committed_files,
                                                    std::mutex &lock, const std::string &ip_addr,
                                                    const int ip_port) {
    sockaddr_in addr_in = {};
    socklen_t addr_len  = {};
    const auto socket   = incoming_socket_multicast(ip_addr, ip_port, addr_in, addr_len);
    const auto addr     = reinterpret_cast<sockaddr *>(&addr_in);
    char incoming_message[MESSAGE_SIZE] = {0};

    do {
        bzero(incoming_message, sizeof(incoming_message));

        // LCOV_EXCL_START
        if (recvfrom(socket, incoming_message, MESSAGE_SIZE, 0, addr, &addr_len) < 0) {
            continue;
        }
        // LCOV_EXCL_STOP

        const auto path = std::string(incoming_message).substr(2);

        if (const char command = incoming_message[0]; command == SET) {
            // Received an advert for a committed file
            std::lock_guard lg(lock);
            if (std::find(committed_files.begin(), committed_files.end(), path) ==
                committed_files.end()) {
                committed_files.emplace_back(path);
            }
        } else {
            // Received a query for a committed file: message begins with capiocl::Monitor::REQUEST
            std::lock_guard lg(lock);
            if (std::find(committed_files.begin(), committed_files.end(), path) !=
                committed_files.end()) {
                _send_message(ip_addr, ip_port, path, SET);
            }
        }
    } while (true);
}

[[noreturn]] void capiocl::monitor::MulticastMonitor::home_node_listener(
    std::unordered_map<std::string, std::string> &home_nodes, std::mutex &lock,
    const std::string &ip_addr, int ip_port) {
    char this_hostname[HOST_NAME_MAX] = {};
    gethostname(this_hostname, HOST_NAME_MAX);

    sockaddr_in addr_in = {};
    socklen_t addr_len  = {};
    const auto socket   = incoming_socket_multicast(ip_addr, ip_port, addr_in, addr_len);

    const auto addr                     = reinterpret_cast<sockaddr *>(&addr_in);
    char incoming_message[MESSAGE_SIZE] = {0};

    do {
        bzero(incoming_message, sizeof(incoming_message));

        // LCOV_EXCL_START
        if (recvfrom(socket, incoming_message, MESSAGE_SIZE, 0, addr, &addr_len) < 0) {
            continue;
        }
        // LCOV_EXCL_STOP

        std::string incoming_message_str(incoming_message);
        std::vector<std::string> tokens;
        size_t start = 0, end = incoming_message_str.find(' ');

        while (end != std::string::npos) {
            if (end > start) {
                tokens.push_back(incoming_message_str.substr(start, end - start));
            }
            start = end + 1;
            end   = incoming_message_str.find(' ', start);
        }

        if (start < incoming_message_str.length()) {
            tokens.push_back(incoming_message_str.substr(start));
        }
        const auto &path = tokens[1];
        if (tokens[0].c_str()[0] == SET) {

            // Received an advert for a committed file
            std::lock_guard lg(lock);

            const auto &home_node = tokens[2];
            home_nodes[path]      = home_node;
        } else {
            // Received a query for a home node, Message begins with capiocl::Monitor::REQUEST
            std::lock_guard lg(lock);

            if (home_nodes.find(path) == home_nodes.end()) {
                continue;
            }

            if (home_nodes[path] == this_hostname) {
                _send_message(ip_addr, ip_port, path + " " + this_hostname, SET);
            }
        }
    } while (true);
}

void capiocl::monitor::MulticastMonitor::_send_message(const std::string &ip_addr,
                                                       const int ip_port, const std::string &path,
                                                       const MESSAGE_COMMANDS action) {
    char message[MESSAGE_SIZE] = {0};
    snprintf(message, sizeof(message), "%c %s", action, path.c_str());
    auto [out_s, addr] = outgoing_socket_multicast(ip_addr, ip_port);
    sendto(out_s, message, strlen(message), 0, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
    close(out_s);
}

capiocl::monitor::MulticastMonitor::MulticastMonitor(
    const configuration::CapioClConfiguration &config) {
    config.getParameter("monitor.mcast.commit.ip", &MULTICAST_COMMIT_ADDR);
    config.getParameter("monitor.mcast.commit.port", &MULTICAST_COMMIT_PORT);
    config.getParameter("monitor.mcast.homenode.ip", &MULTICAST_HOME_NODE_ADDR);
    config.getParameter("monitor.mcast.homenode.port", &MULTICAST_HOME_NODE_PORT);
    config.getParameter("monitor.mcast.delay_ms", &MULTICAST_DELAY_MILLIS);

    commit_thread =
        std::thread(&commit_listener, std::ref(_committed_files), std::ref(committed_lock),
                    MULTICAST_COMMIT_ADDR, MULTICAST_COMMIT_PORT);

    home_node_thread =
        std::thread(&home_node_listener, std::ref(_home_nodes), std::ref(home_node_lock),
                    MULTICAST_HOME_NODE_ADDR, MULTICAST_HOME_NODE_PORT);

    gethostname(_hostname, HOST_NAME_MAX);
}

capiocl::monitor::MulticastMonitor::~MulticastMonitor() {
    pthread_cancel(commit_thread.native_handle());
    pthread_cancel(home_node_thread.native_handle());
    commit_thread.join();
    home_node_thread.join();
}

bool capiocl::monitor::MulticastMonitor::isCommitted(const std::filesystem::path &path) const {

    {
        const std::lock_guard lg(committed_lock);
        if (std::find(_committed_files.begin(), _committed_files.end(), path) !=
            _committed_files.end()) {
            return true;
        }
    }

    _send_message(MULTICAST_COMMIT_ADDR, MULTICAST_COMMIT_PORT, path, GET);
    std::this_thread::sleep_for(std::chrono::milliseconds(MULTICAST_DELAY_MILLIS));

    {
        const std::lock_guard lg(committed_lock);
        return std::find(_committed_files.begin(), _committed_files.end(), path) !=
               _committed_files.end();
    }
}

void capiocl::monitor::MulticastMonitor::setCommitted(const std::filesystem::path &path) const {
    _send_message(MULTICAST_COMMIT_ADDR, MULTICAST_COMMIT_PORT, std::filesystem::path(path), SET);
    std::lock_guard lg(committed_lock);
    const auto position = std::find(_committed_files.begin(), _committed_files.end(), path);
    if (position == _committed_files.end()) {
        _committed_files.emplace_back(path);
    }
}

void capiocl::monitor::MulticastMonitor::setHomeNode(const std::filesystem::path &path) const {
    const std::string message = path.string() + " " + _hostname;
    _send_message(MULTICAST_HOME_NODE_ADDR, MULTICAST_HOME_NODE_PORT, message, SET);

    std::lock_guard lg(home_node_lock);
    _home_nodes[path] = _hostname;
}

const std::string &
capiocl::monitor::MulticastMonitor::getHomeNode(const std::filesystem::path &path) const {

    {
        const std::lock_guard lg(home_node_lock);
        if (const auto itm = _home_nodes.find(path); itm != _home_nodes.end()) {
            return itm->second;
        }
    }

    _send_message(MULTICAST_HOME_NODE_ADDR, MULTICAST_HOME_NODE_PORT, path.string(), GET);
    std::this_thread::sleep_for(std::chrono::milliseconds(MULTICAST_DELAY_MILLIS));

    const std::lock_guard lg(home_node_lock);
    if (const auto itm = _home_nodes.find(path); itm != _home_nodes.end()) {
        return itm->second;
    } else {
        return NO_HOME_NODE;
    }
}