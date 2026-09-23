#include <algorithm>
#include <arpa/inet.h>
#include <cctype>
#include <charconv>
#include <limits>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "calf/StlLogger.h"
#include "capiocl.hpp"
#include "capiocl/monitor.h"


std::atomic<unsigned long> close_count_origin_sequence{0};

static std::tuple<int, sockaddr_in> outgoing_socket_multicast(const std::string &address,
                                                              const int port) {
    START_LOG(calf_current_tid(), "call()");
    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = inet_addr(address.c_str());
    addr.sin_port        = htons(port);

    const int transmission_socket = socket(AF_INET, SOCK_DGRAM, 0);
    // LCOV_EXCL_START
    if (transmission_socket < 0) {
        const int error = errno;
        LOG("multicast socket failed direction=outgoing address=%s port=%d errno=%d",
            address.c_str(), port, error);
        throw capiocl::monitor::MonitorException(std::string("socket() failed: ") +
                                                 strerror(error));
    }
    // LCOV_EXCL_STOP

    return {transmission_socket, addr};
}

static int incoming_socket_multicast(const std::string &address_ip, const int port,
                                     sockaddr_in &addr, socklen_t &addrlen) {
    START_LOG(calf_current_tid(), "call()");
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
        LOG("multicast socket failed direction=incoming address=%s port=%d errno=%d",
            address_ip.c_str(), port, errno);
        throw capiocl::monitor::MonitorException(std::string("socket() failed: ") +
                                                 strerror(errno));
    }

    // Allow multiple sockets to bind to the same port
    if (setsockopt(_socket, SOL_SOCKET, SO_REUSEPORT, &multi_bind, sizeof(multi_bind)) < 0) {
        const int error = errno;
        LOG("multicast setup failed operation=SO_REUSEPORT address=%s port=%d errno=%d",
            address_ip.c_str(), port, error);
        close(_socket);
        throw capiocl::monitor::MonitorException(std::string("REUSEPORT failed: ") +
                                                 strerror(error));
    }

    // Bind to port
    if (bind(_socket, reinterpret_cast<sockaddr *>(&addr), addrlen) < 0) {
        const int error = errno;
        LOG("multicast setup failed operation=bind address=%s port=%d errno=%d", address_ip.c_str(),
            port, error);
        close(_socket);
        throw capiocl::monitor::MonitorException(std::string("bind failed: ") + strerror(error));
    }

    // Join multicast group
    if (setsockopt(_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        const int error = errno;
        LOG("multicast setup failed operation=IP_ADD_MEMBERSHIP address=%s port=%d errno=%d",
            address_ip.c_str(), port, error);
        close(_socket);
        throw capiocl::monitor::MonitorException(std::string("join multicast failed: ") +
                                                 strerror(error));
    }

    // Enable loopback
    if (setsockopt(_socket, IPPROTO_IP, IP_MULTICAST_LOOP, &loopback, sizeof(loopback)) < 0) {
        const int error = errno;
        LOG("multicast setup failed operation=IP_MULTICAST_LOOP address=%s port=%d errno=%d",
            address_ip.c_str(), port, error);
        close(_socket);
        throw capiocl::monitor::MonitorException(std::string("loopback failed: ") +
                                                 strerror(error));
    }
    // LCOV_EXCL_STOP

    return _socket;
}

void capiocl::monitor::MulticastMonitor::commit_listener(
    std::vector<std::string> &committed_files, std::mutex &lock,
    std::unordered_map<std::string, std::unordered_map<std::string, std::uint64_t>> &close_counts,
    std::mutex &close_count_lock, const std::string &ip_addr, const int ip_port,
    const std::atomic<bool> *terminate) {
    START_LOG(calf_current_tid(), "call()");
    sockaddr_in addr_in = {};
    socklen_t addr_len  = {};
    int socket;
    try {
        socket = incoming_socket_multicast(ip_addr, ip_port, addr_in, addr_len);
    } catch (const MonitorException &) {
        return;
    }
    const auto addr                         = reinterpret_cast<sockaddr *>(&addr_in);
    char incoming_message[MESSAGE_SIZE + 1] = {0};

    // Polling for non blocking
    pollfd pfd = {};
    pfd.fd     = socket;
    pfd.events = POLLIN | POLLPRI;

    do {
        if (*terminate) {
            close(socket);
            return;
        }
        bzero(incoming_message, sizeof(incoming_message));

        // TODO: migrate to epoll for linux and kqueue on MacOS
        if (poll(&pfd, 1, MULTICAST_THREAD_POLL_INTERVAL) == 0) {
            continue;
        }

        // LCOV_EXCL_START
        const auto incoming_size = recvfrom(socket, incoming_message, sizeof(incoming_message),
                                            MSG_DONTWAIT, addr, &addr_len);
        if (incoming_size < 0) {
            continue;
        }
        // LCOV_EXCL_STOP

        if (incoming_size > MESSAGE_SIZE) {
            LOG("multicast message discarded reason=oversize size=%zd limit=%d", incoming_size,
                MESSAGE_SIZE);
            continue;
        }
        const std::string msg(incoming_message, static_cast<size_t>(incoming_size));
        if (msg.size() < 3 || msg[1] != ' ') {
            continue;
        }
        const auto path = msg.substr(2);

        if (const char command = incoming_message[0]; command == SET) {
            // Received an advert for a committed file
            std::lock_guard lg(lock);
            if (std::find(committed_files.begin(), committed_files.end(), path) ==
                committed_files.end()) {
                committed_files.emplace_back(path);
                LOG("received committed path=%s total=%zu", path.c_str(), committed_files.size());
            }
        } else if (command == GET) {
            // Received a query for a committed file: message begins with capiocl::Monitor::REQUEST
            std::lock_guard lg(lock);
            if (std::find(committed_files.begin(), committed_files.end(), path) !=
                committed_files.end()) {
                _send_message(ip_addr, ip_port, path, SET);
            }
        } else if (command == COUNT_SET) {
            const auto origin_end = path.find(' ');
            const auto count_end  = origin_end == std::string::npos ? std::string::npos
                                                                    : path.find(' ', origin_end + 1);
            if (origin_end == 0 || count_end == std::string::npos || count_end == origin_end + 1 ||
                count_end + 1 >= path.size()) {
                continue;
            }
            const auto origin = path.substr(0, origin_end);
            if (origin.find_first_of(" \t\r\n") != std::string::npos) {
                continue;
            }
            std::uint64_t count   = 0;
            const auto count_text = path.substr(origin_end + 1, count_end - origin_end - 1);
            const auto parsed =
                std::from_chars(count_text.data(), count_text.data() + count_text.size(), count);
            if (count == 0 || parsed.ec != std::errc{} ||
                parsed.ptr != count_text.data() + count_text.size()) {
                continue;
            }
            const auto count_path =
                std::filesystem::absolute(path.substr(count_end + 1)).lexically_normal().string();
            std::lock_guard count_guard(close_count_lock);
            auto &known = close_counts[count_path][origin];
            known       = std::max(known, count);
        } else if (command == COUNT_GET && !path.empty()) {
            const auto count_path = std::filesystem::absolute(path).lexically_normal().string();
            std::vector<std::pair<std::string, std::uint64_t>> snapshots;
            {
                std::lock_guard count_guard(close_count_lock);
                if (const auto known = close_counts.find(count_path); known != close_counts.end()) {
                    snapshots.assign(known->second.begin(), known->second.end());
                }
            }
            for (const auto &[origin, count] : snapshots) {
                _send_message(ip_addr, ip_port,
                              origin + " " + std::to_string(count) + " " + count_path, COUNT_SET);
            }
        }
    } while (true);
}

void capiocl::monitor::MulticastMonitor::home_node_listener(
    std::unordered_map<std::string, std::string> &home_nodes, std::mutex &lock,
    const std::string &ip_addr, int ip_port, const std::atomic<bool> *terminate) {
    START_LOG(calf_current_tid(), "call()");
    char this_hostname[HOSTNAME_BUFFER_SIZE] = {};
    gethostname(this_hostname, sizeof(this_hostname));
    this_hostname[sizeof(this_hostname) - 1] = '\0';

    sockaddr_in addr_in = {};
    socklen_t addr_len  = {};
    int socket;
    try {
        socket = incoming_socket_multicast(ip_addr, ip_port, addr_in, addr_len);
    } catch (const MonitorException &) {
        return;
    }

    const auto addr                         = reinterpret_cast<sockaddr *>(&addr_in);
    char incoming_message[MESSAGE_SIZE + 1] = {0};

    do {
        if (*terminate) {
            close(socket);
            return;
        }
        bzero(incoming_message, sizeof(incoming_message));

        // Polling for non blocking
        pollfd pfd = {};
        pfd.fd     = socket;
        pfd.events = POLLIN | POLLPRI;

        // TODO: migrate to epoll for linux and kqueue on MacOS
        if (poll(&pfd, 1, MULTICAST_THREAD_POLL_INTERVAL) == 0) {
            continue;
        }

        // LCOV_EXCL_START
        const auto incoming_size = recvfrom(socket, incoming_message, sizeof(incoming_message),
                                            MSG_DONTWAIT, addr, &addr_len);
        if (incoming_size < 0) {
            continue;
        }
        // LCOV_EXCL_STOP

        if (incoming_size > MESSAGE_SIZE) {
            LOG("multicast home-node message discarded reason=oversize size=%zd limit=%d",
                incoming_size, MESSAGE_SIZE);
            continue;
        }
        std::string incoming_message_str(incoming_message, incoming_size);
        if (incoming_message_str.size() < 3 || incoming_message_str[1] != ' ') {
            continue;
        }
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

        // Drop anything that isn't a well-formed message.
        if (tokens.empty()) {
            continue;
        }

        if (const char command = tokens[0].c_str()[0]; command == SET) {
            if (tokens.size() < 3) {
                // need "! <path> <host>" -> malformed message, skip
                continue;
            }
            const auto &path      = tokens[1];
            const auto &home_node = tokens[2];
            std::lock_guard lg(lock);
            home_nodes[path] = home_node;
            LOG("received home node path=%s node=%s", path.c_str(), home_node.c_str());
        } else if (command == GET) {
            // Received a query for a home node, Message begins with capiocl::Monitor::REQUEST
            if (tokens.size() < 2) {
                // need "? <path>" -> malformed message, skip
                continue;
            }
            const auto &path = tokens[1];
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
                                                       const int ip_port,
                                                       const std::string &payload,
                                                       const MESSAGE_COMMANDS action) {
    START_LOG(calf_current_tid(), "call()");
    const std::string message = static_cast<char>(action) + std::string(" ") + payload;
    if (message.size() > MESSAGE_SIZE) {
        LOG("multicast message rejected reason=oversize size=%zu limit=%d", message.size(),
            MESSAGE_SIZE);
        return;
    }
    auto [out_s, addr] = outgoing_socket_multicast(ip_addr, ip_port);
    if (sendto(out_s, message.data(), message.size(), 0, reinterpret_cast<sockaddr *>(&addr),
               sizeof(addr)) < 0) {
        LOG("multicast send failed address=%s port=%d action=%c path=%s errno=%d", ip_addr.c_str(),
            ip_port, action, payload.c_str(), errno);
    } else {
        LOG("multicast message sent address=%s port=%d action=%c path=%s", ip_addr.c_str(), ip_port,
            action, payload.c_str());
    }
    close(out_s);
}

capiocl::monitor::MulticastMonitor::MulticastMonitor(
    const configuration::CapioClConfiguration &config) {
    START_LOG(calf_current_tid(), "call()");
    config.getParameter("monitor.mcast.commit.ip", &MULTICAST_COMMIT_ADDR);
    config.getParameter("monitor.mcast.commit.port", &MULTICAST_COMMIT_PORT);
    config.getParameter("monitor.mcast.homenode.ip", &MULTICAST_HOME_NODE_ADDR);
    config.getParameter("monitor.mcast.homenode.port", &MULTICAST_HOME_NODE_PORT);
    config.getParameter("monitor.mcast.delay_ms", &MULTICAST_DELAY_MILLIS);
    gethostname(_hostname, sizeof(_hostname));
    _hostname[sizeof(_hostname) - 1] = '\0';
    std::string origin_host(_hostname);
    std::replace_if(
        origin_host.begin(), origin_host.end(),
        [](const unsigned char character) { return std::isspace(character); }, '_');
    close_count_origin =
        origin_host + ":" + std::to_string(getpid()) + ":" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ":" +
        std::to_string(close_count_origin_sequence++);
    LOG("multicast monitor configured commit=%s:%d home_node=%s:%d delay_ms=%d",
        MULTICAST_COMMIT_ADDR.c_str(), MULTICAST_COMMIT_PORT, MULTICAST_HOME_NODE_ADDR.c_str(),
        MULTICAST_HOME_NODE_PORT, MULTICAST_DELAY_MILLIS);

    commit_thread =
        std::thread(&commit_listener, std::ref(_committed_files), std::ref(committed_lock),
                    std::ref(close_counts), std::ref(close_count_lock), MULTICAST_COMMIT_ADDR,
                    MULTICAST_COMMIT_PORT, &this->terminate);

    home_node_thread =
        std::thread(&home_node_listener, std::ref(_home_nodes), std::ref(home_node_lock),
                    MULTICAST_HOME_NODE_ADDR, MULTICAST_HOME_NODE_PORT, &this->terminate);
}

capiocl::monitor::MulticastMonitor::~MulticastMonitor() {
    START_LOG(calf_current_tid(), "call()");
    terminate = true;
    commit_thread.join();
    home_node_thread.join();
}

bool capiocl::monitor::MulticastMonitor::isCommitted(const std::filesystem::path &path) const {
    START_LOG(calf_current_tid(), "call()");
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

std::optional<bool>
capiocl::monitor::MulticastMonitor::increaseCloseCount(const std::filesystem::path &path,
                                                       const long threshold) const {
    START_LOG(calf_current_tid(), "call()");
    if (threshold <= 1) {
        throw std::invalid_argument("Persistent ON_CLOSE threshold must be greater than one");
    }
    const auto normalized = std::filesystem::absolute(path).lexically_normal().string();
    const auto reached    = [&] {
        std::uint64_t remaining = static_cast<std::uint64_t>(threshold);
        if (const auto known = close_counts.find(normalized); known != close_counts.end()) {
            for (const auto &[_, count] : known->second) {
                if (count >= remaining) {
                    return true;
                }
                remaining -= count;
            }
        }
        return false;
    };

    std::uint64_t snapshot;
    bool threshold_reached;
    {
        std::lock_guard guard(close_count_lock);
        auto &own_count = close_counts[normalized][close_count_origin];
        if (own_count == std::numeric_limits<std::uint64_t>::max()) {
            throw MonitorException("Multicast close counter overflow for " + normalized);
        }
        snapshot          = ++own_count;
        threshold_reached = reached();
    }
    _send_message(MULTICAST_COMMIT_ADDR, MULTICAST_COMMIT_PORT,
                  close_count_origin + " " + std::to_string(snapshot) + " " + normalized,
                  COUNT_SET);
    if (threshold_reached) {
        return true;
    }

    _send_message(MULTICAST_COMMIT_ADDR, MULTICAST_COMMIT_PORT, normalized, COUNT_GET);
    std::this_thread::sleep_for(std::chrono::milliseconds(MULTICAST_DELAY_MILLIS));
    std::lock_guard guard(close_count_lock);
    return reached();
}

void capiocl::monitor::MulticastMonitor::setCommitted(const std::filesystem::path &path) const {
    START_LOG(calf_current_tid(), "call()");
    _send_message(MULTICAST_COMMIT_ADDR, MULTICAST_COMMIT_PORT, std::filesystem::path(path), SET);
    std::lock_guard lg(committed_lock);
    const auto position = std::find(_committed_files.begin(), _committed_files.end(), path);
    if (position == _committed_files.end()) {
        _committed_files.emplace_back(path);
    }
}

void capiocl::monitor::MulticastMonitor::setHomeNode(const std::filesystem::path &path) const {
    START_LOG(calf_current_tid(), "call()");
    const std::string message = path.string() + " " + _hostname;
    _send_message(MULTICAST_HOME_NODE_ADDR, MULTICAST_HOME_NODE_PORT, message, SET);

    std::lock_guard lg(home_node_lock);
    _home_nodes[path] = _hostname;
}

std::string
capiocl::monitor::MulticastMonitor::getHomeNode(const std::filesystem::path &path) const {
    START_LOG(calf_current_tid(), "call()");
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
