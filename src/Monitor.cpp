#include "capiocl/monitor.h"
#include "calf/StlLogger.h"
#include "capiocl.hpp"

capiocl::monitor::MonitorException::MonitorException(const std::string &msg) : message(msg) {
    START_LOG(calf_current_tid(), "call()");
    LOG("monitor error message=%s", msg.c_str());
    std::cerr << msg << std::endl;
}

bool capiocl::monitor::Monitor::isCommitted(const std::filesystem::path &path) const {
    START_LOG(calf_current_tid(), "call()");
    return std::any_of(interfaces.begin(), interfaces.end(),
                       [&path](const auto &interface) { return interface->isCommitted(path); });
}

void capiocl::monitor::Monitor::setCommitted(std::filesystem::path path) const {
    START_LOG(calf_current_tid(), "call()");
    LOG("setting committed path=%s backends=%zu", path.string().c_str(), interfaces.size());
    std::for_each(interfaces.begin(), interfaces.end(),
                  [&path](const auto &interface) { interface->setCommitted(path); });
}

void capiocl::monitor::Monitor::registerMonitorBackend(const MonitorInterface *interface) {
    START_LOG(calf_current_tid(), "call()");
    interfaces.emplace_back(interface);
    LOG("registered monitor backend total=%zu", interfaces.size());
}

void capiocl::monitor::Monitor::setHomeNode(const std::filesystem::path &path) const {
    START_LOG(calf_current_tid(), "call()");
    LOG("setting home node path=%s backends=%zu", path.string().c_str(), interfaces.size());
    std::for_each(interfaces.begin(), interfaces.end(),
                  [&path](const auto &interface) { interface->setHomeNode(path); });
}

std::set<std::string>
capiocl::monitor::Monitor::getHomeNode(const std::filesystem::path &path) const {
    START_LOG(calf_current_tid(), "call()");
    std::set<std::string> home_nodes;
    for (const auto &interface : interfaces) {
        const auto node = interface->getHomeNode(path);
        if (node == NO_HOME_NODE) {
            continue;
        }
        home_nodes.insert(node);
    }
    return home_nodes;
}

capiocl::monitor::Monitor::~Monitor() {
    START_LOG(calf_current_tid(), "call()");
    for (const auto &interface : interfaces) {
        delete interface;
    }
}
