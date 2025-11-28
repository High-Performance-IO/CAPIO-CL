#include "capiocl.hpp"
#include "capiocl/monitor.h"

bool capiocl::monitor::MonitorInterface::isCommitted(const std::filesystem::path &path) const {
    std::string msg = "Attempted to use MonitorInterface as Monitor backend to check commit for: ";
    msg += path.string();
    throw MonitorException(msg);
}

void capiocl::monitor::MonitorInterface::setCommitted(const std::filesystem::path &path) const {
    std::string msg = "Attempted to use MonitorInterface as Monitor backend to set commit for: ";
    msg += path.string();
    throw MonitorException(msg);
}
void capiocl::monitor::MonitorInterface::setHomeNode(const std::filesystem::path &path) const {
    std::string msg = "Attempted to use MonitorInterface as Monitor backend to set commit for: ";
    msg += path.string();
    throw MonitorException(msg);
}

const std::string &
capiocl::monitor::MonitorInterface::getHomeNode(const std::filesystem::path &path) const {
    std::string msg = "Attempted to use MonitorInterface as Monitor backend to set commit for: ";
    msg += path.string();
    throw MonitorException(msg);
}