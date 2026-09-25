#include "capiocl/configuration.h"

ConfigurationEntry capiocl::configuration::defaults::DEFAULT_MONITOR_MCAST_IP{
    "capiocl.monitor.mcast.commit.ip", "224.224.224.1"};

ConfigurationEntry capiocl::configuration::defaults::DEFAULT_MONITOR_MCAST_PORT{
    "capiocl.monitor.mcast.commit.port", "12345"};

ConfigurationEntry capiocl::configuration::defaults::DEFAULT_MONITOR_MCAST_DELAY{
    "capiocl.monitor.mcast.delay_ms", "300"};

ConfigurationEntry capiocl::configuration::defaults::DEFAULT_MONITOR_HOMENODE_IP{
    "capiocl.monitor.mcast.homenode.ip", "224.224.224.2"};

ConfigurationEntry capiocl::configuration::defaults::DEFAULT_MONITOR_HOMENODE_PORT{
    "capiocl.monitor.mcast.homenode.port", "12345"};

ConfigurationEntry capiocl::configuration::defaults::DEFAULT_MONITOR_MCAST_ENABLED{
    "capiocl.monitor.mcast.enabled", "true"};

ConfigurationEntry capiocl::configuration::defaults::DEFAULT_MONITOR_FS_ENABLED{
    "capiocl.monitor.filesystem.enabled", "true"};

ConfigurationEntry capiocl::configuration::defaults::DEFAULT_MONITOR_FS_METADATA_DIR{
    "capiocl.monitor.filesystem.metadata_dir", ""};

ConfigurationEntry capiocl::configuration::defaults::DEFAULT_API_MULTICAST_IP{"capiocl.dynamic_api.ip",
                                                                              "224.224.224.3"};

ConfigurationEntry capiocl::configuration::defaults::DEFAULT_API_MULTICAST_PORT{"capiocl.dynamic_api.port",
                                                                                "11223"};
