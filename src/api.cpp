#include <arpa/inet.h>
#include <condition_variable>
#include <iostream>
#include <jsoncons/json.hpp>
#include <mutex>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include "calf/StdOutLogger.h"
#include "capiocl/api.h"
#include "capiocl/engine.h"

std::mutex _setupMtx;
std::condition_variable _setupCv;
bool thread_ready = false;

/// @brief Main WebServer thread function
void server(const std::string &address, const int port, capiocl::engine::Engine *engine,
            std::atomic<bool> *terminate) {

    UPDATE_CALF_WORKFLOW_NAME(engine->getWorkflowName());

    constexpr int RECV_BUF_SIZE = 65535;

    const auto &wf_name = engine->getWorkflowName();

    int fd = socket(AF_INET, SOCK_DGRAM, 0);

    int reuse = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in localAddr{};
    localAddr.sin_family      = AF_INET;
    localAddr.sin_port        = htons(port);
    localAddr.sin_addr.s_addr = INADDR_ANY;

    bind(fd, reinterpret_cast<sockaddr *>(&localAddr), sizeof(localAddr));

    ip_mreq group{};
    group.imr_multiaddr.s_addr = inet_addr(address.c_str());
    group.imr_interface.s_addr = INADDR_ANY;
    setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &group, sizeof(group));

    char buffer[RECV_BUF_SIZE] = {0};
    sockaddr_in srcAddr{};
    socklen_t addrlen = sizeof(srcAddr);

    // timeout of 1 second for termination
    timeval tv{};
    tv.tv_sec  = 0;
    tv.tv_usec = 500;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    thread_ready = true;
    _setupCv.notify_all();

    while (!*terminate) {
        // GCOVR_EXCL_START
        ssize_t n = recvfrom(fd, buffer, RECV_BUF_SIZE - 1, 0,
                             reinterpret_cast<sockaddr *>(&srcAddr), &addrlen);
        // GCOVR_EXCL_STOP

        if (n <= 0) {
            continue;
        }

        buffer[n] = '\0';

        try {
            auto data = jsoncons::json::parse(buffer); // GCOVR_EXCL_LINE
            const auto path =
                data.get_value_or<std::string, std::string>("path", ""); // GCOVR_EXCL_LINE
            if (path.empty()) {
                continue;
            }
            const auto workflow_name =
                data.get_value_or<std::string, std::string>("workflow_name", ""); // GCOVR_EXCL_LINE
            if (workflow_name.empty()) {
                continue;
            }
            const auto jsonEntry =
                data.get_value_or<std::string, std::string>("CapioClEntry", ""); // GCOVR_EXCL_LINE
            if (jsonEntry.empty()) {
                continue;
            }
            auto rule = capiocl::engine::CapioCLEntry::fromJson(jsonEntry);

            if (workflow_name == wf_name) {
                engine->add(path, rule);
            } else {
                continue;
            }

        } catch (const jsoncons::json_exception &e) {
            CALF_PRINT_COLOR(CALF_CLI_LEVEL_ERROR, "APIServer: Received invalid json: %s",
                             e.what());
        }
    }

    close(fd);
}

capiocl::api::CapioClApiServer::CapioClApiServer(engine::Engine *engine,
                                                 configuration::CapioClConfiguration &config)
    : capiocl_configuration(config) {

    UPDATE_CALF_WORKFLOW_NAME(engine->getWorkflowName());

    std::string address;
    int port;
    try {
        config.getParameter("dynamic_api.ip", &address); // GCOVR_EXCL_LINE
    } catch (...) {
        address = configuration::defaults::DEFAULT_API_MULTICAST_IP.v;
    }

    try {
        config.getParameter("dynamic_api.port", &port); // GCOVR_EXCL_LINE
    } catch (...) {
        port = std::stoi(configuration::defaults::DEFAULT_API_MULTICAST_PORT.v);
    }

    _webApiThread = std::thread(server, address, port, engine, &_terminate);

    std::unique_lock lock(_setupMtx);
    _setupCv.wait(lock, [] { return thread_ready; });

    CALF_PRINT_COLOR(CALF_CLI_LEVEL_INFO, "API server @ %s:%d", address.c_str(), port);
}

capiocl::api::CapioClApiServer::~CapioClApiServer() {
    _terminate = true;
    _webApiThread.join();
}
