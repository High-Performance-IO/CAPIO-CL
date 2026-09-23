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
    const auto normalized = std::filesystem::absolute(path).lexically_normal();
    return std::any_of(interfaces.begin(), interfaces.end(), [&normalized](const auto &interface) {
        return interface->isCommitted(normalized);
    });
}

void capiocl::monitor::Monitor::setCommitted(std::filesystem::path path) const {
    START_LOG(calf_current_tid(), "call()");
    path = std::filesystem::absolute(path).lexically_normal();
    LOG("setting committed path=%s backends=%zu", path.string().c_str(), interfaces.size());
    std::for_each(interfaces.begin(), interfaces.end(),
                  [&path](const auto &interface) { interface->setCommitted(path); });
}

bool capiocl::monitor::Monitor::increaseCloseCount(const std::filesystem::path &path,
                                                   const long threshold) const {
    START_LOG(calf_current_tid(), "call()");
    if (threshold <= 1) {
        throw std::invalid_argument("Persistent ON_CLOSE threshold must be greater than one");
    }
    const auto normalized = std::filesystem::absolute(path).lexically_normal();
    bool supported        = false;
    bool reached          = false;
    for (const auto &interface : interfaces) {
        if (const auto committed = interface->increaseCloseCount(normalized, threshold);
            committed.has_value()) {
            supported = true;
            reached |= *committed;
        }
    }
    if (!supported) {
        throw MonitorException("ON_CLOSE count greater than one requires a counting monitor");
    }
    if (reached) {
        setCommitted(normalized);
    }
    return reached;
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
