#include "capiocl.hpp"
#include <arpa/inet.h>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>

#define MESSAGE_SIZE (2 + PATH_MAX)

std::filesystem::path compute_commit_token_name(const std::filesystem::path &path) {
    const auto abs = std::filesystem::absolute(path);
    auto token     = abs.parent_path() / ("." + abs.filename().string() + ".capiocl");
    return token;
}

void generate_commit_token(const std::filesystem::path &path) {
    if (const auto token_name = compute_commit_token_name(path);
        !std::filesystem::exists(token_name)) {
        std::filesystem::create_directories(token_name.parent_path());
        std::ofstream file(token_name);
        file.close();
    }
}

static std::tuple<int, sockaddr_in> outgoing_socket_multicast(const std::string &address,
                                                              const int port) {
    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = inet_addr(address.c_str());
    addr.sin_port        = htons(port);

    const int transmission_socket = socket(AF_INET, SOCK_DGRAM, 0);
    // LCOV_EXCL_START
    if (transmission_socket < 0) {
        throw capiocl::MonitorException(std::string("socket() failed: ") + strerror(errno));
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
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addrlen              = sizeof(addr);

    ip_mreq mreq              = {};
    mreq.imr_multiaddr.s_addr = inet_addr(address_ip.c_str());
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    const int _socket = socket(AF_INET, SOCK_DGRAM, 0);

    // LCOV_EXCL_START
    if (_socket < 0) {
        throw capiocl::MonitorException(std::string("socket() failed: ") + strerror(errno));
    }

    // Allow multiple sockets to bind to the same addr
    if (setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, &multi_bind, sizeof(multi_bind)) < 0) {
        throw capiocl::MonitorException(std::string("REUSEADDR failed: ") + strerror(errno));
    }

    // Allow multiple sockets to bind to the same port
    if (setsockopt(_socket, SOL_SOCKET, SO_REUSEPORT, &multi_bind, sizeof(multi_bind)) < 0) {
        throw capiocl::MonitorException(std::string("REUSEPORT failed: ") + strerror(errno));
    }

    // Bind to port
    if (bind(_socket, reinterpret_cast<sockaddr *>(&addr), addrlen) < 0) {
        throw capiocl::MonitorException(std::string("bind failed: ") + strerror(errno));
    }

    // Join multicast group
    if (setsockopt(_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        throw capiocl::MonitorException(std::string("join multicast failed: ") + strerror(errno));
    }

    // Enable loopback
    if (setsockopt(_socket, IPPROTO_IP, IP_MULTICAST_LOOP, &loopback, sizeof(loopback)) < 0) {
        throw capiocl::MonitorException(std::string("loopback failed: ") + strerror(errno));
    }
    // LCOV_EXCL_STOP

    return _socket;
}

void capiocl::Monitor::commit_listener(std::vector<std::string> &committed_files, std::mutex &lock,
                                       const bool *continue_execution, const std::string &ip_addr,
                                       const int ip_port) {
    sockaddr_in addr_in = {};
    socklen_t addr_len  = {};
    const auto socket   = incoming_socket_multicast(ip_addr, ip_port, addr_in, addr_len);
    print_message(CLI_LEVEL_INFO, "Monitor on thread: " + std::to_string(getpid()));

    const auto addr                     = reinterpret_cast<sockaddr *>(&addr_in);
    char incoming_message[MESSAGE_SIZE] = {0};

    do {
        bzero(incoming_message, sizeof(incoming_message));

        // LCOV_EXCL_START
        if (recvfrom(socket, incoming_message, MESSAGE_SIZE, 0, addr, &addr_len) < 0) {
            continue;
        }
        // LCOV_EXCL_STOP

        const auto path = std::string(incoming_message).substr(2);

        if (const char command = incoming_message[0]; command == COMMIT) {
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
                _send_message(ip_addr, ip_port, path, COMMIT);
            }
        }
    } while (*continue_execution);
}

void capiocl::Monitor::_send_message(const std::string &ip_addr, const int ip_port,
                                     const std::string &path, const MESSAGE_COMMANDS action) {
    char message[MESSAGE_SIZE] = {0};
    snprintf(message, sizeof(message), "%c %s", action, path.c_str());
    auto [out_s, addr] = outgoing_socket_multicast(ip_addr, ip_port);
    sendto(out_s, message, strlen(message), 0, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
    close(out_s);
}

capiocl::Monitor::Monitor(const std::string &ip_addr, const int ip_port) {
    continue_execution = new bool(true);
    MULTICAST_ADDR     = ip_addr;
    MULTICAST_PORT     = ip_port;

    commit_listener_thread = new std::thread(&Monitor::commit_listener, std::ref(_committed_files),
                                             std::ref(committed_lock), std::ref(continue_execution),
                                             MULTICAST_ADDR, MULTICAST_PORT);
}

capiocl::Monitor::~Monitor() {
    *continue_execution = false;
    pthread_cancel(commit_listener_thread->native_handle());
    commit_listener_thread->join();
    delete commit_listener_thread;
    delete continue_execution;
}

bool capiocl::Monitor::isCommitted(const std::filesystem::path &path) const {

    bool found;
    {
        const std::lock_guard lg(committed_lock);
        found = std::find(_committed_files.begin(), _committed_files.end(), path) !=
                _committed_files.end();
    }

    if (found) {
        return true;
    } else {
        _send_message(MULTICAST_ADDR, MULTICAST_PORT, path, REQUEST);
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        {
            const std::lock_guard lg(committed_lock);
            found = std::find(_committed_files.begin(), _committed_files.end(), path) !=
                    _committed_files.end();
        }
        if (found) {
            return true;
        } else {
            return std::filesystem::exists(compute_commit_token_name(path));
        }
    }
}

void capiocl::Monitor::setCommitted(const std::filesystem::path path) const {
    _send_message(MULTICAST_ADDR, MULTICAST_PORT, std::filesystem::path(path), COMMIT);
    std::lock_guard lg(committed_lock);
    if (std::find(_committed_files.begin(), _committed_files.end(), path) ==
        _committed_files.end()) {
        _committed_files.emplace_back(path);
        generate_commit_token(path);
    }
}
