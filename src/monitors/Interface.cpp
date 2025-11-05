#include "capiocl.hpp"

bool capiocl::MonitorInterface::isCommitted(const std::filesystem::path &path) const {
    std::string msg = "Attempted to use MonitorInterface as Monitor backend to check commit for: ";
    msg += path.string();
    throw MonitorException(msg);
}

void capiocl::MonitorInterface::setCommitted(const std::filesystem::path &path) const {
    std::string msg = "Attempted to use MonitorInterface as Monitor backend to set commit for: ";
    msg += path.string();
    throw MonitorException(msg);
}