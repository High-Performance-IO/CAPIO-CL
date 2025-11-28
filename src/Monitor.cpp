#include "capiocl/monitor.h"
#include "capiocl.hpp"
#include "capiocl/printer.h"

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

std::set<std::string>
capiocl::monitor::Monitor::getHomeNode(const std::filesystem::path &path) const {
    std::set<std::string> home_nodes;
    for (const auto &interface : interfaces) {
        const auto &node = interface->getHomeNode(path);
        if (node == NO_HOME_NODE) {
            continue;
        }
        home_nodes.insert(node);
    }
    return home_nodes;
}

capiocl::monitor::Monitor::~Monitor() {
    for (const auto &interface : interfaces) {
        delete interface;
    }
}