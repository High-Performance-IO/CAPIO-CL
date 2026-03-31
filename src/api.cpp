#include <arpa/inet.h>
#include <iostream>
#include <jsoncons/json.hpp>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "capiocl/api.h"
#include "capiocl/engine.h"
#include "capiocl/printer.h"

/// @brief Main WebServer thread function
void server(const std::string &address, const int port, capiocl::engine::Engine *engine) {
    pthread_setcancelstate(PTHREAD_CANCEL_ASYNCHRONOUS, nullptr);

    capiocl::printer::print(capiocl::printer::CLI_LEVEL_INFO,
                            "Starting API server @ " + address + ":" + std::to_string(port));

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("Socket creation failed");
        return;
    }

    int reuse = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("Setting SO_REUSEADDR failed");
        close(fd);
        return;
    }

    sockaddr_in localAddr{};
    localAddr.sin_family      = AF_INET;
    localAddr.sin_port        = htons(port);
    localAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(fd, reinterpret_cast<sockaddr *>(&localAddr), sizeof(localAddr)) < 0) {
        perror("Bind failed");
        close(fd);
        return;
    }

    ip_mreq group{};
    group.imr_multiaddr.s_addr = inet_addr(address.c_str());
    group.imr_interface.s_addr = INADDR_ANY;
    if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &group, sizeof(group)) < 0) {
        perror("Adding multicast group failed");
        close(fd);
        return;
    }

    char buffer[65535] = {0};
    sockaddr_in srcAddr{};
    socklen_t addrlen = sizeof(srcAddr);

    // timeout of 1 second for termination
    timeval tv{};
    tv.tv_sec  = 1;
    tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    while (true) {
        ssize_t nbytes = recvfrom(fd, buffer, sizeof(buffer) - 1, 0,
                                  reinterpret_cast<struct sockaddr *>(&srcAddr), &addrlen);
        if (nbytes < 0) {
            break;
        }

        buffer[nbytes] = '\0';

        try {
            auto data          = jsoncons::json::parse(buffer);
            auto path          = data["path"].as_string();
            auto workflow_name = data["workflow_name"].as_string();
            auto rule = capiocl::engine::CapioCLEntry::fromJson(data["CapioClEntry"].as_string());

            if (workflow_name == engine->getWorkflowName()) {
                engine->add(path, rule);
            }

        } catch (const jsoncons::json_exception &e) {
            capiocl::printer::print(capiocl::printer::CLI_LEVEL_ERROR,
                                    "JSON Parse Error: " + std::string(e.what()));
        }
    }

    close(fd);
}

capiocl::api::CapioClApiServer::CapioClApiServer(engine::Engine *engine,
                                                 configuration::CapioClConfiguration &config)
    : capiocl_configuration(config) {

    std::string address;
    config.getParameter("dynamic_api.ip", &address);

    int port;
    config.getParameter("dynamic_api.port", &port);

    _webApiThread = std::thread(server, address, port, engine);
}

capiocl::api::CapioClApiServer::~CapioClApiServer() {
    pthread_cancel(_webApiThread.native_handle());
    _webApiThread.join();
}
