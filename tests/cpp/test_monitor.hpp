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

    try {
        interface.setCommitted("/test");
    } catch (capiocl::monitor::MonitorException &e) {
        EXPECT_TRUE(strlen(e.what()) > 50);
    }
}

#endif // CAPIO_CL_MONITOR_HPP