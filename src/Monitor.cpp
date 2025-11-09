#include "include/monitor.h"
#include "capiocl.hpp"
#include "include/printer.h"

capiocl::monitor::MonitorException::MonitorException(const std::string &msg) : message(msg) {
    printer::print(printer::CLI_LEVEL_ERROR, msg);
}

bool capiocl::monitor::Monitor::isCommitted(const std::filesystem::path &path) const {
    return std::any_of(interfaces.begin(), interfaces.end(),
                       [&path](const auto &interface) { return interface->isCommitted(path); });
}

void capiocl::monitor::Monitor::setCommitted(std::filesystem::path path) const {
    std::for_each(interfaces.begin(), interfaces.end(),
                  [&path](const auto &interface) { interface->setCommitted(path); });
}

void capiocl::monitor::Monitor::registerMonitorBackend(const MonitorInterface *interface) {
    interfaces.emplace_back(interface);
}
void capiocl::monitor::Monitor::setHomeNode(const std::filesystem::path &path) const {
    std::for_each(interfaces.begin(), interfaces.end(),
                  [&path](const auto &interface) { interface->setHomeNode(path); });
}

const std::vector<std::string>
capiocl::monitor::Monitor::getHomeNode(const std::filesystem::path path) const {
    std::vector<std::string> home_nodes;
    for (const auto &interface : interfaces) {
        if (auto home_node = interface->getHomeNode(path);
            std::find(home_nodes.begin(), home_nodes.end(), home_node) == home_nodes.end()) {
            home_nodes.emplace_back(home_node);
        }
    }
    return home_nodes;
}

capiocl::monitor::Monitor::~Monitor() {
    for (const auto &interface : interfaces) {
        delete interface;
    }
}