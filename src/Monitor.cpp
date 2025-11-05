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

capiocl::monitor::Monitor::~Monitor() {
    for (const auto &interface : interfaces) {
        delete interface;
    }
}