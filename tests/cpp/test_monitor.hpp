#ifndef CAPIO_CL_MONITOR_HPP
#define CAPIO_CL_MONITOR_HPP

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
    std::mutex m1;
    std::lock_guard lg(m1);
    std::thread t1([&]() {
        const capiocl::Engine e;
        e.setCommitted("a");
        EXPECT_TRUE(e.isCommitted("a"));
        std::lock_guard lg1(m1);
    });

    sleep(1);

    std::thread t2([&]() {
        const capiocl::Engine e;

        while (!e.isCommitted("a")) {
            sleep(1);
        }
        EXPECT_TRUE(e.isCommitted("a"));
    });

    t1.join();
    t2.join();
}
#endif // CAPIO_CL_MONITOR_HPP
