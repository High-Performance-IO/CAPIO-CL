#include "capiocl.hpp"

bool capiocl::Monitor::isCommitted(const std::filesystem::path &path) const {
    return std::any_of(interfaces.begin(), interfaces.end(),
                       [&path](const auto &interface) { return interface->isCommitted(path); });
}
void capiocl::Monitor::setCommitted(std::filesystem::path path) const {
    std::for_each(interfaces.begin(), interfaces.end(),
                  [&path](const auto &interface) { interface->setCommitted(path); });
}
void capiocl::Monitor::registerMonitorBackend(const MonitorInterface *interface) {
    interfaces.emplace_back(interface);
}
capiocl::Monitor::~Monitor() {
    for (const auto &interface : interfaces) {
        delete interface;
    }
}
