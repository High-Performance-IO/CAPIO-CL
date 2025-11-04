#ifndef CAPIO_CL_MONITOR_HPP
#define CAPIO_CL_MONITOR_HPP

#define MONITOR_SUITE_NAME testMonitor

inline void commit_monit_test_f1() {
    capiocl::Engine e;

    while (!e.isCommitted("a")) {
        sleep(1);
    }

    EXPECT_TRUE(e.isCommitted("a"));
}

inline void commit_monit_test_f2() {
    capiocl::Engine e;
    sleep(1);
    e.setCommitted("a");
    EXPECT_TRUE(e.isCommitted("a"));
}

TEST(MONITOR_SUITE_NAME, testCommitCommunication) {
    std::thread t1(commit_monit_test_f1);
    std::thread t2(commit_monit_test_f2);

    t1.join();
    t2.join();
}

#endif // CAPIO_CL_MONITOR_HPP
