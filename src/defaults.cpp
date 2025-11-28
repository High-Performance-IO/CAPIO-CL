#include "capiocl//configuration.h"

ConfigurationEntry capiocl::configuration::defaults::DEFAULT_MONITOR_MCAST_IP{
    "monitor.mcast.commit.ip", "224.224.224.1"};

ConfigurationEntry capiocl::configuration::defaults::DEFAULT_MONITOR_MCAST_PORT{
    "monitor.mcast.commit.port", "12345"};

ConfigurationEntry capiocl::configuration::defaults::DEFAULT_MONITOR_MCAST_DELAY{
    "monitor.mcast.delay_ms", "300"};

ConfigurationEntry capiocl::configuration::defaults::DEFAULT_MONITOR_HOMENODE_IP{
    "monitor.mcast.homenode.ip", "224.224.224.2"};

ConfigurationEntry capiocl::configuration::defaults::DEFAULT_MONITOR_HOMENODE_PORT{
    "monitor.mcast.homenode.port", "12345"};
