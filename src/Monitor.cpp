#include "capiocl.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

static int outgoing_socket_multicast(const std::string &address, const int port,
                                     sockaddr_in *addr) {
    int transmission_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (transmission_socket < 0) {
        return -1;
    }

    addr->sin_family      = AF_INET;
    addr->sin_addr.s_addr = inet_addr(address.c_str());
    addr->sin_port        = htons(port);
    return transmission_socket;
};

static int incoming_socket_multicast(const std::string &address_ip, const int port,
                                     sockaddr_in &addr, socklen_t &addrlen) {
    constexpr int loopback   = 1; // enable reception of loopback messages
    constexpr int multi_bind = 1; // enable multiple sockets on same address

    const int _socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (_socket < 0) {
        throw capiocl::MonitorException(std::string("socket() failed: ") + strerror(errno));
    }

    // Allow multiple sockets to bind to the same port
    if (setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, &multi_bind, sizeof(multi_bind)) < 0) {
        throw capiocl::MonitorException(std::string("REUSEADDR failed: ") + strerror(errno));
    }

    if (setsockopt(_socket, SOL_SOCKET, SO_REUSEPORT, &multi_bind, sizeof(multi_bind)) < 0) {
        throw capiocl::MonitorException(std::string("REUSEPORT failed: ") + strerror(errno));
    }

    // Bind to port
    addr                 = {};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addrlen              = sizeof(addr);

    if (bind(_socket, reinterpret_cast<sockaddr *>(&addr), addrlen) < 0) {
        throw capiocl::MonitorException(std::string("bind failed: ") + strerror(errno));
    }

    // Join multicast group
    ip_mreq mreq              = {};
    mreq.imr_multiaddr.s_addr = inet_addr(address_ip.c_str());
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        throw capiocl::MonitorException(std::string("join multicast failed: ") + strerror(errno));
    }

    // Enable loopback
    if (setsockopt(_socket, IPPROTO_IP, IP_MULTICAST_LOOP, &loopback, sizeof(loopback)) < 0) {
        throw capiocl::MonitorException(std::string("loopback failed: ") + strerror(errno));
    }

    return _socket;
}

void capiocl::Monitor::commit_listener(std::vector<std::string> &committed_files, std::mutex &lock,
                                       const bool *continue_execution, const std::string &ip_addr,
                                       const int ip_port) {
    sockaddr_in addr_in = {};
    socklen_t addr_len  = {};
    const auto socket   = incoming_socket_multicast(ip_addr, ip_port, addr_in, addr_len);
    print_message(CLI_LEVEL_INFO, "Commit Monitor thread started");

    const auto addr                 = reinterpret_cast<sockaddr *>(&addr_in);
    char incoming_message[PATH_MAX] = {0};

    do {
        bzero(incoming_message, sizeof(incoming_message));

        if (recvfrom(socket, incoming_message, PATH_MAX, 0, addr, &addr_len) < 0) {
            continue;
        }
        {
            std::lock_guard lg(lock);
            if (std::find(committed_files.begin(), committed_files.end(), incoming_message) ==
                committed_files.end()) {
                committed_files.emplace_back(incoming_message);
            }
        }
    } while (*continue_execution);
}

capiocl::Monitor::Monitor() {
    continue_execution = new bool(true);
    MULTICAST_ADDR     = "224.224.224.1";
    MULTICAST_PORT     = 12345;

    commit_listener_thread = new std::thread(&Monitor::commit_listener, std::ref(_committed_files),
                                             std::ref(committed_lock), std::ref(continue_execution),
                                             MULTICAST_ADDR, MULTICAST_PORT);
}

capiocl::Monitor::~Monitor() { delete commit_listener_thread; }

bool capiocl::Monitor::isCommitted(const std::filesystem::path &path) const {
    const std::lock_guard lg(committed_lock);
    if (std::find(_committed_files.begin(), _committed_files.end(), path) !=
        _committed_files.end()) {
        return true;
    } else {
        return false;
    }
}

void capiocl::Monitor::setCommitted(const std::filesystem::path &path) const {
    sockaddr_in addr       = {};
    char message[PATH_MAX] = {0};
    memcpy(message, path.c_str(), PATH_MAX);
    const auto socket = outgoing_socket_multicast(MULTICAST_ADDR, MULTICAST_PORT, &addr);
    if (sendto(socket, message, strlen(message), 0, reinterpret_cast<sockaddr *>(&addr),
               sizeof(addr)) < 0) {
        print_message(CLI_LEVEL_ERROR, std::string("Unable to send message to multicast group: ") +
                                           std::strerror(errno));
    };
    close(socket);
    std::lock_guard lg(committed_lock);
    if (std::find(_committed_files.begin(), _committed_files.end(), path) ==
        _committed_files.end()) {
        _committed_files.emplace_back(path);
    }
}
