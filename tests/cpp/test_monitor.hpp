#ifndef CAPIO_CL_MONITOR_HPP
#define CAPIO_CL_MONITOR_HPP

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define MONITOR_SUITE_NAME testMonitor

TEST(MONITOR_SUITE_NAME, testCommitCommunication) {
    std::thread t1([]() {
        const capiocl::engine::Engine e;

        while (!e.isCommitted("a")) {
            sleep(1);
        }

        EXPECT_TRUE(e.isCommitted("a"));
    });

    std::thread t2([]() {
        const capiocl::engine::Engine e;
        sleep(1);
        e.setCommitted("a");
        EXPECT_TRUE(e.isCommitted("a"));
    });

    t1.join();
    t2.join();
}

TEST(MONITOR_SUITE_NAME, testIsCommittedAfterStartup) {
    const capiocl::engine::Engine e;
    e.setCommitted("a");
    EXPECT_TRUE(e.isCommitted("a"));

    sleep(1);

    const capiocl::engine::Engine e1;
    while (!e1.isCommitted("a")) {
        sleep(1);
    }
    EXPECT_TRUE(e1.isCommitted("a"));
}

TEST(MONITOR_SUITE_NAME, testCommitAfterTerminationOfServer) {
    const auto *e = new capiocl::engine::Engine();
    e->setCommitted("a");
    delete e;
    sleep(1);
    const capiocl::engine::Engine e1;
    EXPECT_TRUE(e1.isCommitted("a"));
}

TEST(MONITOR_SUITE_NAME, testIssueExceptionOnMonitorInstance) {
    capiocl::monitor::MonitorInterface interface;
    EXPECT_THROW(interface.isCommitted("/test"), capiocl::monitor::MonitorException);
    EXPECT_THROW(interface.setCommitted("/test"), capiocl::monitor::MonitorException);
    EXPECT_THROW(interface.setHomeNode("./test.txt"), capiocl::monitor::MonitorException);
    EXPECT_THROW(interface.getHomeNode("./test.txt"), capiocl::monitor::MonitorException);

    try {
        interface.setCommitted("/test");
    } catch (capiocl::monitor::MonitorException &e) {
        EXPECT_TRUE(strlen(e.what()) > 50);
    }
}

TEST(MONITOR_SUITE_NAME, testHomeNodeAcrossDifferentThreads) {
    const auto e1 = new capiocl::engine::Engine();
    const auto e2 = new capiocl::engine::Engine();

    char hostname[HOST_NAME_MAX] = {};
    gethostname(hostname, HOST_NAME_MAX);

    e1->setHomeNode("test.txt");
    const std::set<std::string> home_nodes  = e2->getHomeNode("test.txt");
    // test for entry already present.
    const std::set<std::string> home_nodes2 = e2->getHomeNode("test.txt");

    EXPECT_EQ(home_nodes.size(), 1);
    EXPECT_NE(home_nodes.find(hostname), home_nodes.end());
    EXPECT_NE(home_nodes2.find(hostname), home_nodes2.end());

    const auto home_nodes_1 = e2->getHomeNode("test1.txt");
    EXPECT_EQ(home_nodes_1.size(), 0);
}

TEST(MONITOR_SUITE_NAME, testHomeNodeAfterSetup) {
    char hostname[HOST_NAME_MAX] = {};
    gethostname(hostname, HOST_NAME_MAX);

    const capiocl::engine::Engine e;
    e.setHomeNode("test.txt");

    sleep(1);

    const capiocl::engine::Engine e1;
    const auto home_nodes = e1.getHomeNode("test.txt");

    EXPECT_EQ(home_nodes.size(), 1);
    EXPECT_NE(home_nodes.find(hostname), home_nodes.end());
}

TEST(MONITOR_SUITE_NAME, testHomeNodeAfterInstanceTearDown) {
    char hostname[HOST_NAME_MAX] = {};
    gethostname(hostname, HOST_NAME_MAX);

    auto e1 = new capiocl::engine::Engine();

    e1->setHomeNode("test.txt");
    delete e1;

    sleep(1);

    const capiocl::engine::Engine e2;
    const std::set<std::string> home_nodes = e2.getHomeNode("test.txt");

    EXPECT_EQ(home_nodes.size(), 1);
    EXPECT_NE(home_nodes.find(hostname), home_nodes.end());
}

#endif // CAPIO_CL_MONITOR_HPP