#ifndef CAPIO_CL_RUNTIME_COMMIT_HPP
#define CAPIO_CL_RUNTIME_COMMIT_HPP

#include <atomic>
#include <arpa/inet.h>
#include <fcntl.h>
#include <fstream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define RUNTIME_COMMIT_SUITE_NAME testRuntimeCommit

namespace {
class RuntimeTestScope final {
    static std::atomic<unsigned> sequence;
    bool had_metadata_root = false;
    std::string previous_metadata_root;

    static std::string hexEncode(const std::string &value) {
        static constexpr char digits[] = "0123456789abcdef";
        std::string encoded;
        for (const unsigned char character : value) {
            encoded.push_back(digits[character >> 4]);
            encoded.push_back(digits[character & 0x0f]);
        }
        return encoded;
    }

  public:
    std::filesystem::path root;
    std::filesystem::path data;
    std::filesystem::path metadata;

    RuntimeTestScope()
        : root(std::filesystem::temp_directory_path() /
               ("capiocl-runtime-" + std::to_string(getpid()) + "-" +
                std::to_string(sequence++))),
          data(root / "data"), metadata(root / "metadata") {
        if (const char *current = std::getenv("CAPIO_METADATA_DIR")) {
            had_metadata_root      = true;
            previous_metadata_root = current;
        }
        std::filesystem::remove_all(root);
        std::filesystem::create_directories(data);
        setenv("CAPIO_METADATA_DIR", metadata.c_str(), 1);
    }

    ~RuntimeTestScope() {
        if (had_metadata_root) {
            setenv("CAPIO_METADATA_DIR", previous_metadata_root.c_str(), 1);
        } else {
            unsetenv("CAPIO_METADATA_DIR");
        }
        std::filesystem::remove_all(root);
    }

    [[nodiscard]] std::filesystem::path file(const std::string &name) const {
        return data / name;
    }

    [[nodiscard]] std::filesystem::path closeMetadata(const std::filesystem::path &path,
                                                      const std::string &name) const {
        const auto encoded = hexEncode(
            std::filesystem::absolute(path).lexically_normal().generic_string());
        auto directory = metadata / "capiocl" / "close-counts";
        for (size_t offset = 0; offset < encoded.size(); offset += 64) {
            directory /= encoded.substr(offset, 64);
        }
        return directory / name;
    }

    [[nodiscard]] std::filesystem::path closeLock(const std::filesystem::path &path) const {
        const auto encoded = hexEncode(
            std::filesystem::absolute(path).lexically_normal().generic_string());
        auto directory = metadata / "capiocl" / "close-locks";
        for (size_t offset = 0; offset < encoded.size(); offset += 64) {
            directory /= encoded.substr(offset, 64);
        }
        return directory / "lock";
    }
};

std::atomic<unsigned> RuntimeTestScope::sequence{0};

void useFilesystemMonitor(capiocl::engine::Engine &engine) {
    engine.loadConfiguration("/tmp/capio_cl_tomls/runtime_fs_only.toml");
}

void useMulticastMonitor(capiocl::engine::Engine &engine) {
    engine.loadConfiguration("/tmp/capio_cl_tomls/runtime_multicast_only.toml");
}

void sendMulticastCount(const std::string &message) {
    const int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    ASSERT_NE(socket_fd, -1);
    sockaddr_in address{};
    address.sin_family      = AF_INET;
    address.sin_addr.s_addr = inet_addr("224.224.226.10");
    address.sin_port        = htons(26101);
    ASSERT_EQ(sendto(socket_fd, message.data(), message.size(), 0,
                     reinterpret_cast<sockaddr *>(&address), sizeof(address)),
              static_cast<ssize_t>(message.size()));
    close(socket_fd);
}

void configureClose(capiocl::engine::Engine &engine, const std::filesystem::path &path,
                    const long threshold) {
    engine.setCommitRule(path, capiocl::commitRules::ON_CLOSE);
    engine.setCommitedCloseNumber(path, threshold);
}

void configureFile(capiocl::engine::Engine &engine, const std::filesystem::path &path,
                   const std::vector<std::filesystem::path> &dependencies) {
    engine.setCommitRule(path, capiocl::commitRules::ON_FILE);
    if (!dependencies.empty()) {
        engine.setFileDeps(path, dependencies);
    }
}
} // namespace

TEST(RUNTIME_COMMIT_SUITE_NAME, onClosePlainAndOneCommitOnFirstClose) {
    RuntimeTestScope scope;
    for (const long threshold : {0L, 1L}) {
        const auto path = scope.file("plain-" + std::to_string(threshold));
        capiocl::engine::Engine engine(false);
        useFilesystemMonitor(engine);
        configureClose(engine, path, threshold);

        EXPECT_TRUE(engine.increaseCloseCount(path));
        EXPECT_TRUE(engine.isCommitted(path));
        EXPECT_FALSE(std::filesystem::exists(scope.closeMetadata(path, "count")));
    }
}

TEST(RUNTIME_COMMIT_SUITE_NAME, countedCloseRequiresConfiguredMetadataRoot) {
    RuntimeTestScope scope;
    unsetenv("CAPIO_METADATA_DIR");
    const auto counted = scope.file("missing-metadata");
    capiocl::engine::Engine engine(false);
    useFilesystemMonitor(engine);
    configureClose(engine, counted, 2);

    try {
        engine.increaseCloseCount(counted);
        FAIL() << "counted ON_CLOSE accepted a missing CAPIO_METADATA_DIR";
    } catch (const capiocl::monitor::MonitorException &error) {
        EXPECT_NE(std::string(error.what()).find("CAPIO_METADATA_DIR"), std::string::npos);
    }
    EXPECT_FALSE(engine.isCommitted(counted));
    EXPECT_FALSE(std::filesystem::exists(counted.parent_path() /
                                         ("." + counted.filename().string() + ".commit")));

    const auto plain = scope.file("plain-without-metadata");
    configureClose(engine, plain, 1);
    EXPECT_TRUE(engine.increaseCloseCount(plain));
    EXPECT_TRUE(engine.isCommitted(plain));
}

TEST(RUNTIME_COMMIT_SUITE_NAME, onCloseThresholdTwoAndNonClose) {
    RuntimeTestScope scope;
    const auto path = scope.file("threshold-two");
    capiocl::engine::Engine engine(false);
    useFilesystemMonitor(engine);
    configureClose(engine, path, 2);

    EXPECT_FALSE(engine.increaseCloseCount(path));
    EXPECT_TRUE(std::filesystem::exists(scope.closeMetadata(path, "count")));
    EXPECT_NE(scope.closeMetadata(path, "count").parent_path(),
              scope.closeLock(path).parent_path());
    EXPECT_FALSE(std::filesystem::exists(scope.closeLock(path)));
    EXPECT_TRUE(engine.increaseCloseCount(path));
    EXPECT_FALSE(std::filesystem::exists(scope.closeLock(path)));
    EXPECT_TRUE(engine.increaseCloseCount(path));

    const auto other = scope.file("not-on-close");
    EXPECT_FALSE(engine.increaseCloseCount(other));
    EXPECT_FALSE(std::filesystem::exists(scope.closeMetadata(other, "count")));
}

TEST(RUNTIME_COMMIT_SUITE_NAME, onCloseCountPersistsAcrossEngines) {
    RuntimeTestScope scope;
    const auto path = scope.file("persistent");
    {
        capiocl::engine::Engine engine(false);
        useFilesystemMonitor(engine);
        configureClose(engine, path, 3);
        EXPECT_FALSE(engine.increaseCloseCount(path));
        EXPECT_FALSE(engine.increaseCloseCount(path));
    }
    {
        capiocl::engine::Engine engine(false);
        useFilesystemMonitor(engine);
        configureClose(engine, path, 3);
        EXPECT_TRUE(engine.increaseCloseCount(path));
    }
}

TEST(RUNTIME_COMMIT_SUITE_NAME, onCloseIsThreadSafe) {
    RuntimeTestScope scope;
    const auto path = scope.file("threads");
    capiocl::engine::Engine engine(false);
    useFilesystemMonitor(engine);
    configureClose(engine, path, 16);
    std::vector<std::thread> threads;
    for (int i = 0; i < 16; ++i) {
        threads.emplace_back([&] { engine.increaseCloseCount(path); });
    }
    for (auto &thread : threads) {
        thread.join();
    }
    EXPECT_TRUE(engine.isCommitted(path));
    EXPECT_FALSE(std::filesystem::exists(scope.closeLock(path)));
}

TEST(RUNTIME_COMMIT_SUITE_NAME, multicastCloseCountAcrossOriginsAndSpaces) {
    RuntimeTestScope scope;
    const auto path = scope.file("multicast path with spaces");
    capiocl::engine::Engine first(false), second(false);
    useMulticastMonitor(first);
    useMulticastMonitor(second);
    configureClose(first, path, 2);
    configureClose(second, path, 2);

    EXPECT_FALSE(first.increaseCloseCount(path));
    EXPECT_TRUE(second.increaseCloseCount(path));
    EXPECT_TRUE(first.isCommitted(path));
}

TEST(RUNTIME_COMMIT_SUITE_NAME, multicastCloseCountReconcilesLateObserver) {
    RuntimeTestScope scope;
    const auto path = scope.file("late-observer");
    capiocl::engine::Engine first(false);
    useMulticastMonitor(first);
    configureClose(first, path, 2);
    EXPECT_FALSE(first.increaseCloseCount(path));

    capiocl::engine::Engine second(false);
    useMulticastMonitor(second);
    configureClose(second, path, 2);
    EXPECT_TRUE(second.increaseCloseCount(path));
}

TEST(RUNTIME_COMMIT_SUITE_NAME, multicastSnapshotsMergeByMaximum) {
    RuntimeTestScope scope;
    const auto path = scope.file("snapshot-merge");
    capiocl::engine::Engine engine(false);
    useMulticastMonitor(engine);
    configureClose(engine, path, 6);
    usleep(50000);
    const auto normalized = std::filesystem::absolute(path).lexically_normal().string();
    sendMulticastCount("C injected 5 " + normalized);
    sendMulticastCount("C injected 2 " + normalized);

    EXPECT_TRUE(engine.increaseCloseCount(path));
}

TEST(RUNTIME_COMMIT_SUITE_NAME, multicastRejectsMalformedAndOversizedMessages) {
    RuntimeTestScope scope;
    const auto path = scope.file("malformed-multicast");
    const auto normalized = std::filesystem::absolute(path).lexically_normal().string();
    capiocl::engine::Engine engine(false);
    useMulticastMonitor(engine);
    configureClose(engine, path, 2);
    usleep(50000);

    sendMulticastCount("!" + normalized);
    sendMulticastCount("C injected 18446744073709551616 " + normalized);
    sendMulticastCount("C injected 1junk " + normalized);
    EXPECT_FALSE(engine.increaseCloseCount(path));

    const std::string oversized_path = "/" + std::string(8191, 'x');
    sendMulticastCount("! " + oversized_path + "x");
    EXPECT_FALSE(engine.isCommitted(oversized_path));
}

TEST(RUNTIME_COMMIT_SUITE_NAME, multicastConcurrentClosesReconcile) {
    RuntimeTestScope scope;
    const auto path = scope.file("concurrent-multicast");
    capiocl::engine::Engine first(false), second(false);
    useMulticastMonitor(first);
    useMulticastMonitor(second);
    configureClose(first, path, 2);
    configureClose(second, path, 2);

    std::atomic<int> ready{0};
    std::atomic<bool> start{false};
    bool first_result = false, second_result = false;
    std::thread first_close([&] {
        ++ready;
        while (!start) {
            std::this_thread::yield();
        }
        first_result = first.increaseCloseCount(path);
    });
    std::thread second_close([&] {
        ++ready;
        while (!start) {
            std::this_thread::yield();
        }
        second_result = second.increaseCloseCount(path);
    });
    while (ready != 2) {
        std::this_thread::yield();
    }
    start = true;
    first_close.join();
    second_close.join();

    EXPECT_TRUE(first_result || second_result);
    EXPECT_TRUE(first.isCommitted(path));
}

TEST(RUNTIME_COMMIT_SUITE_NAME, mixedBackendsDoNotDoubleCount) {
    RuntimeTestScope scope;
    const auto path = scope.file("mixed-backends");
    capiocl::engine::Engine engine(false);
    engine.loadConfiguration("/tmp/capio_cl_tomls/runtime_mixed_monitors.toml");
    configureClose(engine, path, 2);

    EXPECT_FALSE(engine.increaseCloseCount(path));
    EXPECT_TRUE(engine.increaseCloseCount(path));
}

TEST(RUNTIME_COMMIT_SUITE_NAME, simultaneousFirstCloseIsProcessSafe) {
    RuntimeTestScope scope;
    const auto path = scope.file("processes");
    int start[2];
    ASSERT_EQ(pipe(start), 0);
    std::vector<pid_t> children;
    for (int i = 0; i < 2; ++i) {
        const pid_t child = fork();
        ASSERT_NE(child, -1);
        if (child == 0) {
            close(start[1]);
            char signal;
            if (read(start[0], &signal, 1) != 1) {
                _exit(2);
            }
            try {
                capiocl::engine::Engine engine(false);
                useFilesystemMonitor(engine);
                configureClose(engine, path, 2);
                engine.increaseCloseCount(path);
                _exit(0);
            } catch (...) {
                _exit(3);
            }
        }
        children.push_back(child);
    }
    close(start[0]);
    ASSERT_EQ(write(start[1], "xx", 2), 2);
    close(start[1]);
    for (const pid_t child : children) {
        int status;
        ASSERT_EQ(waitpid(child, &status, 0), child);
        EXPECT_TRUE(WIFEXITED(status));
        EXPECT_EQ(WEXITSTATUS(status), 0);
    }

    capiocl::engine::Engine engine(false);
    useFilesystemMonitor(engine);
    configureClose(engine, path, 2);
    EXPECT_TRUE(engine.isCommitted(path));
    EXPECT_FALSE(std::filesystem::exists(scope.closeLock(path)));
}

TEST(RUNTIME_COMMIT_SUITE_NAME, concurrentExplicitCommitWinsBelowThreshold) {
    RuntimeTestScope scope;
    const auto path = scope.file("explicit-race");
    const auto lock_path = scope.closeLock(path);
    std::filesystem::create_directories(lock_path.parent_path());
    int ready[2], release[2];
    ASSERT_EQ(pipe(ready), 0);
    ASSERT_EQ(pipe(release), 0);
    const pid_t locker = fork();
    ASSERT_NE(locker, -1);
    if (locker == 0) {
        close(ready[0]);
        close(release[1]);
        const int lock_fd =
            open(lock_path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
        if (lock_fd == -1 || write(ready[1], "r", 1) != 1) {
            _exit(2);
        }
        char signal;
        if (read(release[0], &signal, 1) != 1) {
            _exit(3);
        }
        close(lock_fd);
        unlink(lock_path.c_str());
        _exit(0);
    }
    close(ready[1]);
    close(release[0]);
    char signal;
    ASSERT_EQ(read(ready[0], &signal, 1), 1);
    close(ready[0]);

    capiocl::engine::Engine engine(false);
    useFilesystemMonitor(engine);
    configureClose(engine, path, 10);
    std::atomic<bool> started{false};
    bool result = false;
    std::thread closer([&] {
        started = true;
        result  = engine.increaseCloseCount(path);
    });
    while (!started) {
        std::this_thread::yield();
    }
    usleep(50000);
    engine.setCommitted(path);
    ASSERT_EQ(write(release[1], "x", 1), 1);
    close(release[1]);
    closer.join();
    int status;
    ASSERT_EQ(waitpid(locker, &status, 0), locker);
    ASSERT_TRUE(WIFEXITED(status));
    ASSERT_EQ(WEXITSTATUS(status), 0);
    EXPECT_TRUE(result);
    EXPECT_FALSE(std::filesystem::exists(lock_path));
}

TEST(RUNTIME_COMMIT_SUITE_NAME, relativeAndAbsolutePathsShareRuntimeIdentity) {
    RuntimeTestScope scope;
    const auto absolute = scope.file("alias");
    const auto relative = std::filesystem::relative(absolute, std::filesystem::current_path());
    capiocl::engine::Engine engine(false);
    useFilesystemMonitor(engine);
    configureClose(engine, relative, 2);

    EXPECT_FALSE(engine.increaseCloseCount(absolute));
    EXPECT_TRUE(engine.increaseCloseCount(relative));
    EXPECT_TRUE(engine.isCommitted(absolute));
    EXPECT_TRUE(engine.isCommitted(relative));
}

TEST(RUNTIME_COMMIT_SUITE_NAME, onCloseRejectsMalformedAndUnsupportedCounters) {
    RuntimeTestScope scope;
    const auto malformed = scope.file("malformed");
    capiocl::engine::Engine engine(false);
    useFilesystemMonitor(engine);
    configureClose(engine, malformed, 2);
    const auto counter = scope.closeMetadata(malformed, "count");
    std::filesystem::create_directories(counter.parent_path());
    std::ofstream(counter) << "not-a-number\n";
    EXPECT_THROW(engine.increaseCloseCount(malformed), capiocl::monitor::MonitorException);
    EXPECT_FALSE(std::filesystem::exists(scope.closeLock(malformed)));
    EXPECT_THROW(engine.setCommitedCloseNumber(malformed, -1), std::invalid_argument);
    capiocl::engine::CapioCLEntry invalid;
    invalid.commit_on_close_count = -1;
    EXPECT_THROW(engine.add(malformed, invalid), std::invalid_argument);

    const auto unsupported = scope.file("unsupported");
    capiocl::engine::Engine no_backend(false);
    configureClose(no_backend, unsupported, 2);
    EXPECT_THROW(no_backend.increaseCloseCount(unsupported), capiocl::monitor::MonitorException);
}

TEST(RUNTIME_COMMIT_SUITE_NAME, lockAcquisitionErrorsDoNotSpin) {
    if (geteuid() == 0) {
        GTEST_SKIP() << "permission failure cannot be induced as root";
    }
    RuntimeTestScope scope;
    const auto path = scope.file("lock-error");
    const auto lock = scope.closeLock(path);
    std::filesystem::create_directories(lock.parent_path());
    ASSERT_EQ(chmod(lock.parent_path().c_str(), 0500), 0);

    capiocl::engine::Engine engine(false);
    useFilesystemMonitor(engine);
    configureClose(engine, path, 2);
    EXPECT_THROW(engine.increaseCloseCount(path), capiocl::monitor::MonitorException);

    ASSERT_EQ(chmod(lock.parent_path().c_str(), 0700), 0);
    EXPECT_FALSE(std::filesystem::exists(lock));
}

TEST(RUNTIME_COMMIT_SUITE_NAME, onFileChainAndFanIn) {
    RuntimeTestScope scope;
    const auto a = scope.file("a");
    const auto b = scope.file("b");
    const auto c = scope.file("c");
    const auto d = scope.file("d");
    capiocl::engine::Engine engine(false);
    useFilesystemMonitor(engine);
    configureFile(engine, a, {b, c});
    configureFile(engine, b, {d});

    engine.setCommitted(d);
    EXPECT_FALSE(engine.isCommitted(a));
    engine.setCommitted(c);
    EXPECT_TRUE(engine.isCommitted(a));
    EXPECT_TRUE(engine.isCommitted(b));
}

TEST(RUNTIME_COMMIT_SUITE_NAME, onFileEmptyAndCyclesRemainUncommitted) {
    RuntimeTestScope scope;
    const auto empty = scope.file("empty");
    const auto self  = scope.file("self");
    const auto a     = scope.file("a");
    const auto b     = scope.file("b");
    capiocl::engine::Engine engine(false);
    useFilesystemMonitor(engine);
    configureFile(engine, empty, {});
    configureFile(engine, self, {self});
    configureFile(engine, a, {b});
    configureFile(engine, b, {a});

    EXPECT_FALSE(engine.isCommitted(empty));
    EXPECT_FALSE(engine.isCommitted(self));
    EXPECT_FALSE(engine.isCommitted(a));
    EXPECT_FALSE(engine.isCommitted(b));
}

TEST(RUNTIME_COMMIT_SUITE_NAME, onFileCycleResolvesFromRawSeed) {
    RuntimeTestScope scope;
    const auto a = scope.file("a");
    const auto b = scope.file("b");
    capiocl::engine::Engine engine(false);
    useFilesystemMonitor(engine);
    configureFile(engine, a, {b});
    configureFile(engine, b, {a});

    engine.setCommitted(a);
    EXPECT_TRUE(engine.isCommitted(b));
    EXPECT_TRUE(engine.isCommitted(a));
}

#endif // CAPIO_CL_RUNTIME_COMMIT_HPP
