#ifndef CAPIO_CL_MONITOR_HPP
#define CAPIO_CL_MONITOR_HPP

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define MONITOR_SUITE_NAME testMonitor

TEST(MONITOR_SUITE_NAME, testCommitCommunication) {
    std::thread t1([]() {
        const capiocl::Engine e;

        while (!e.isCommitted("a")) {
            sleep(1);
        }

        EXPECT_TRUE(e.isCommitted("a"));
    });

    std::thread t2([]() {
        const capiocl::Engine e;
        sleep(1);
        e.setCommitted("a");
        EXPECT_TRUE(e.isCommitted("a"));
    });

    t1.join();
    t2.join();
}

TEST(MONITOR_SUITE_NAME, testIsCommittedAfterStartup) {

    const capiocl::Engine e;
    e.setCommitted("a");
    EXPECT_TRUE(e.isCommitted("a"));

    sleep(1);

    const capiocl::Engine e1;
    while (!e1.isCommitted("a")) {
        sleep(1);
    }
    EXPECT_TRUE(e1.isCommitted("a"));
}

TEST(MONITOR_SUITE_NAME, testCommitAfterTerminationOfServer) {
    const auto *e = new capiocl::Engine();
    e->setCommitted("a");
    delete e;
    sleep(1);
    const capiocl::Engine e1;
    EXPECT_TRUE(e1.isCommitted("a"));
}

#endif // CAPIO_CL_MONITOR_HPP
