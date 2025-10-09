#include "capiocl.hpp"
#include <cxxabi.h>
#include <gtest/gtest.h>

TEST(testCapioClEngine, testInstantiation) {
    capiocl::Engine engine;
    EXPECT_EQ(engine.size(), 0);
    engine.print();
}

TEST(testCapioClEngine, testAddFileDefault) {
    capiocl::Engine engine;
    EXPECT_EQ(engine.size(), 0);
    engine.newFile("test.dat");
    EXPECT_EQ(engine.size(), 1);
    EXPECT_EQ(engine.getCommitRule("test.dat"), capiocl::COMMITTED_ON_TERMINATION);
    EXPECT_EQ(engine.getFireRule("test.dat"), capiocl::MODE_UPDATE);
    EXPECT_TRUE(engine.getConsumers("test.dat").empty());
    EXPECT_TRUE(engine.getProducers("test.dat").empty());
    EXPECT_FALSE(engine.isPermanent("test.dat"));
    EXPECT_FALSE(engine.isExcluded("test.dat"));
    EXPECT_TRUE(engine.isFile("test.dat"));
    EXPECT_FALSE(engine.isDirectory("test.dat"));
    EXPECT_EQ(engine.getDirectoryFileCount("test.dat"), 0);
    EXPECT_FALSE(engine.isStoredInMemory("test.dat"));
}

TEST(testCapioClEngine, testAddFileDefaultGlob) {
    capiocl::Engine engine;
    EXPECT_EQ(engine.size(), 0);
    engine.newFile("test.*");
    EXPECT_EQ(engine.size(), 1);
    EXPECT_EQ(engine.getCommitRule("test.dat"), capiocl::COMMITTED_ON_TERMINATION);
    EXPECT_EQ(engine.getFireRule("test.dat"), capiocl::MODE_UPDATE);
    EXPECT_TRUE(engine.getConsumers("test.dat").empty());
    EXPECT_TRUE(engine.getProducers("test.dat").empty());
    EXPECT_FALSE(engine.isPermanent("test.dat"));
    EXPECT_FALSE(engine.isExcluded("test.dat"));
    EXPECT_TRUE(engine.isFile("test.dat"));
    EXPECT_FALSE(engine.isDirectory("test.dat"));
    EXPECT_EQ(engine.getDirectoryFileCount("test.dat"), 0);
    EXPECT_FALSE(engine.isStoredInMemory("test.dat"));
}

TEST(testCapioClEngine, testAddFileDefaultGlobQuestion) {
    capiocl::Engine engine;
    EXPECT_EQ(engine.size(), 0);
    engine.newFile("test.?");
    EXPECT_EQ(engine.size(), 1);
    EXPECT_EQ(engine.getCommitRule("test.1"), capiocl::COMMITTED_ON_TERMINATION);
    EXPECT_EQ(engine.getFireRule("test.1"), capiocl::MODE_UPDATE);
    EXPECT_TRUE(engine.getConsumers("test.1").empty());
    EXPECT_TRUE(engine.getProducers("test.1").empty());
    EXPECT_FALSE(engine.isPermanent("test.1"));
    EXPECT_FALSE(engine.isExcluded("test.1"));
    EXPECT_TRUE(engine.isFile("test.1"));
    EXPECT_FALSE(engine.isDirectory("test.1"));
    EXPECT_EQ(engine.getDirectoryFileCount("test.1"), 0);
    EXPECT_FALSE(engine.isStoredInMemory("test.1"));
}

TEST(testCapioClEngine, testAddFileManually) {
    capiocl::Engine engine;
    EXPECT_EQ(engine.size(), 0);
    std::string path = "test.dat";
    std::vector<std::string> producers, consumers, file_dependencies;

    engine.add(path, producers, consumers, capiocl::COMMITTED_ON_TERMINATION, capiocl::MODE_UPDATE,
               false, false, file_dependencies);
    EXPECT_EQ(engine.size(), 1);
    EXPECT_EQ(engine.getCommitRule("test.dat"), capiocl::COMMITTED_ON_TERMINATION);
    EXPECT_EQ(engine.getFireRule("test.dat"), capiocl::MODE_UPDATE);
    EXPECT_TRUE(engine.getConsumers("test.dat").empty());
    EXPECT_TRUE(engine.getProducers("test.dat").empty());
    EXPECT_FALSE(engine.isPermanent("test.dat"));
    EXPECT_FALSE(engine.isExcluded("test.dat"));
    EXPECT_TRUE(engine.isFile("test.dat"));
    EXPECT_FALSE(engine.isDirectory("test.dat"));
    EXPECT_EQ(engine.getDirectoryFileCount("test.dat"), 0);
    EXPECT_FALSE(engine.isStoredInMemory("test.dat"));

    engine.setFile("test.txt");
}

TEST(testCapioClEngine, testAddFileManuallyGlob) {
    capiocl::Engine engine;
    EXPECT_EQ(engine.size(), 0);
    std::string path = "test.*";
    std::vector<std::string> producers, consumers, file_dependencies;

    engine.add(path, producers, consumers, capiocl::COMMITTED_ON_TERMINATION, capiocl::MODE_UPDATE,
               false, false, file_dependencies);
    EXPECT_EQ(engine.size(), 1);
    EXPECT_EQ(engine.getCommitRule("test.dat"), capiocl::COMMITTED_ON_TERMINATION);
    EXPECT_EQ(engine.getFireRule("test.dat"), capiocl::MODE_UPDATE);
    EXPECT_TRUE(engine.getConsumers("test.dat").empty());
    EXPECT_TRUE(engine.getProducers("test.dat").empty());
    EXPECT_FALSE(engine.isPermanent("test.dat"));
    EXPECT_FALSE(engine.isExcluded("test.dat"));
    EXPECT_TRUE(engine.isFile("test.dat"));
    EXPECT_FALSE(engine.isDirectory("test.dat"));
    EXPECT_EQ(engine.getDirectoryFileCount("test.dat"), 0);
    EXPECT_FALSE(engine.isStoredInMemory("test.dat"));
}

TEST(testCapioClEngine, testAddFileManuallyQuestion) {
    capiocl::Engine engine;
    EXPECT_EQ(engine.size(), 0);
    std::string path = "test.?";
    std::vector<std::string> producers, consumers, file_dependencies;

    engine.add(path, producers, consumers, capiocl::COMMITTED_ON_CLOSE, capiocl::MODE_NO_UPDATE,
               false, false, file_dependencies);
    engine.setDirectory("test.?");
    engine.setDirectoryFileCount("test.?", 10);

    EXPECT_EQ(engine.size(), 1);
    EXPECT_FALSE(engine.getCommitRule("test.dat") == capiocl::COMMITTED_ON_CLOSE);
    EXPECT_FALSE(engine.getFireRule("test.dat") == capiocl::MODE_NO_UPDATE);
    EXPECT_EQ(engine.getCommitRule("test.1"), capiocl::COMMITTED_ON_CLOSE);
    EXPECT_EQ(engine.getFireRule("test.1"), capiocl::MODE_NO_UPDATE);
    EXPECT_EQ(engine.getCommitRule("test.2"), capiocl::COMMITTED_ON_CLOSE);
    EXPECT_EQ(engine.getFireRule("test.2"), capiocl::MODE_NO_UPDATE);
    EXPECT_TRUE(engine.isDirectory("test.1"));
    EXPECT_EQ(engine.getDirectoryFileCount("test.?"), 10);
    EXPECT_EQ(engine.getDirectoryFileCount("test.3"), 10);
    EXPECT_TRUE(engine.getConsumers("test.4").empty());
    EXPECT_TRUE(engine.getProducers("test.5").empty());
    EXPECT_FALSE(engine.isPermanent("test.6"));
    EXPECT_FALSE(engine.isExcluded("test.7"));
    EXPECT_FALSE(engine.isFile("test.8"));
    EXPECT_TRUE(engine.isDirectory("test.9"));
    EXPECT_EQ(engine.getDirectoryFileCount("test.a"), 10);
    EXPECT_FALSE(engine.isStoredInMemory("test.b"));

    engine.setDirectoryFileCount("myDir", 10);
    EXPECT_EQ(engine.getDirectoryFileCount("myDir"), 10);
}

TEST(testCapioClEngine, testAddFileManuallyGlobExplcit) {
    capiocl::Engine engine;
    EXPECT_EQ(engine.size(), 0);
    std::string path = "test.[abc][abc][abc]";
    std::vector<std::string> producers, consumers, file_dependencies;

    engine.add(path, producers, consumers, capiocl::COMMITTED_ON_CLOSE, capiocl::MODE_NO_UPDATE,
               false, false, file_dependencies);
    engine.setDirectory("test.[abc][abc][abc]");
    engine.setDirectoryFileCount("test.[abc][abc][abc]", 10);

    EXPECT_EQ(engine.size(), 1);
    EXPECT_FALSE(engine.getCommitRule("test.dat") == capiocl::COMMITTED_ON_CLOSE);
    EXPECT_FALSE(engine.getFireRule("test.dat") == capiocl::MODE_NO_UPDATE);
    EXPECT_TRUE(engine.getCommitRule("test.abc") == capiocl::COMMITTED_ON_CLOSE);
    EXPECT_TRUE(engine.getFireRule("test.aaa") == capiocl::MODE_NO_UPDATE);
    EXPECT_EQ(engine.getCommitRule("test.cab"), capiocl::COMMITTED_ON_CLOSE);
    EXPECT_EQ(engine.getFireRule("test.bac"), capiocl::MODE_NO_UPDATE);
    EXPECT_EQ(engine.getCommitRule("test.ccc"), capiocl::COMMITTED_ON_CLOSE);
    EXPECT_EQ(engine.getFireRule("test.aaa"), capiocl::MODE_NO_UPDATE);
    EXPECT_TRUE(engine.isDirectory("test.bbb"));
    EXPECT_NE(engine.getDirectoryFileCount("test.3"), 10);
}

TEST(testCapioClEngine, testProducerConsumersFileDependencies) {
    capiocl::Engine engine;
    EXPECT_EQ(engine.size(), 0);
    std::vector<std::string> producers = {"A", "B"}, consumers = {"C", "D"},
                             file_dependencies = {"E", "F"};

    engine.newFile("test.dat");

    engine.addProducer("test.dat", producers[0]);
    EXPECT_EQ(engine.getProducers("test.dat").size(), 1);
    EXPECT_TRUE(engine.isProducer("test.dat", producers[0]));

    engine.addProducer("test.dat", producers[1]);
    EXPECT_EQ(engine.getProducers("test.dat").size(), 2);
    EXPECT_TRUE(engine.isProducer("test.dat", producers[1]));

    engine.addConsumer("test.dat", consumers[0]);
    EXPECT_EQ(engine.getConsumers("test.dat").size(), 1);
    EXPECT_TRUE(engine.isConsumer("test.dat", consumers[0]));

    engine.addConsumer("test.dat", consumers[1]);
    EXPECT_EQ(engine.getConsumers("test.dat").size(), 2);
    EXPECT_TRUE(engine.isConsumer("test.dat", consumers[1]));

    EXPECT_TRUE(engine.getCommitOnFileDependencies("test.dat").empty());
    engine.addFileDependency("test.dat", file_dependencies[0]);
    EXPECT_EQ(engine.getCommitOnFileDependencies("test.dat").size(), 1);
    EXPECT_TRUE(engine.getCommitOnFileDependencies("test.dat")[0] == file_dependencies[0]);
    engine.addFileDependency("test.dat", file_dependencies[1]);
    EXPECT_EQ(engine.getCommitOnFileDependencies("test.dat").size(), 2);
    EXPECT_TRUE(engine.getCommitOnFileDependencies("test.dat")[1] == file_dependencies[1]);

    EXPECT_TRUE(engine.getCommitOnFileDependencies("myNewFile").empty());

    engine.addFileDependency("myFile.txt", file_dependencies[0]);
    EXPECT_TRUE(engine.getCommitRule("myFile.txt") == capiocl::COMMITTED_ON_FILE);
    EXPECT_EQ(engine.getCommitOnFileDependencies("myFile.txt").size(), 1);
    EXPECT_TRUE(engine.getCommitOnFileDependencies("myFile.txt")[0] == file_dependencies[0]);
}

TEST(testCapioClEngine, testProducerConsumersFileDependenciesGlob) {
    capiocl::Engine engine;
    EXPECT_EQ(engine.size(), 0);
    std::vector<std::string> producers = {"A", "B"}, consumers = {"C", "D"},
                             file_dependencies = {"E", "F"};

    engine.newFile("test.*");

    engine.addProducer("test.dat", producers[0]);
    EXPECT_EQ(engine.getProducers("test.dat").size(), 1);
    EXPECT_TRUE(engine.isProducer("test.dat", producers[0]));

    engine.addProducer("test.dat", producers[1]);
    EXPECT_EQ(engine.getProducers("test.dat").size(), 2);
    EXPECT_TRUE(engine.isProducer("test.dat", producers[1]));

    engine.addConsumer("test.dat", consumers[0]);
    EXPECT_EQ(engine.getConsumers("test.dat").size(), 1);
    EXPECT_TRUE(engine.isConsumer("test.dat", consumers[0]));

    engine.addConsumer("test.dat", consumers[1]);
    EXPECT_EQ(engine.getConsumers("test.dat").size(), 2);
    EXPECT_TRUE(engine.isConsumer("test.dat", consumers[1]));

    EXPECT_TRUE(engine.getCommitOnFileDependencies("test.dat").empty());
    engine.addFileDependency("test.dat", file_dependencies[0]);
    EXPECT_EQ(engine.getCommitOnFileDependencies("test.dat").size(), 1);
    EXPECT_TRUE(engine.getCommitOnFileDependencies("test.dat")[0] == file_dependencies[0]);
    engine.addFileDependency("test.dat", file_dependencies[1]);
    EXPECT_EQ(engine.getCommitOnFileDependencies("test.dat").size(), 2);
    EXPECT_TRUE(engine.getCommitOnFileDependencies("test.dat")[1] == file_dependencies[1]);
}

TEST(testCapioClEngine, testCommitFirePermanentExcludeOnGlobs) {
    capiocl::Engine engine;
    engine.newFile("test.*");
    engine.setFireRule("test.*", capiocl::MODE_NO_UPDATE);

    EXPECT_TRUE(engine.isFirable("test.a"));
    EXPECT_FALSE(engine.isFirable("testb"));

    engine.setCommitRule("testb", capiocl::COMMITTED_ON_FILE);
    engine.setFileDeps("testb", {"test.a"});

    engine.setPermanent("myFile", true);
    EXPECT_TRUE(engine.isPermanent("myFile"));
    EXPECT_FALSE(engine.isFirable("myFile"));

    EXPECT_FALSE(engine.isExcluded("testb"));
    engine.setExclude("testb", true);
    EXPECT_TRUE(engine.isExcluded("testb"));
    engine.setExclude("testb", true);
    EXPECT_TRUE(engine.isExcluded("testb"));
    engine.setExclude("myFile.*", true);
    EXPECT_TRUE(engine.isExcluded("myFile.txt"));
    EXPECT_TRUE(engine.isExcluded("myFile.dat"));

    engine.setFireRule("test.c", capiocl::MODE_NO_UPDATE);
    EXPECT_TRUE(engine.isFirable("test.c"));

    engine.setCommitedCloseNumber("test.e", 100);
}

TEST(testCapioClEngine, testIsFileIsDirectoryGlob) {
    capiocl::Engine engine;
    engine.newFile("test.*");
    engine.setDirectory("test.d/");
    engine.setDirectory("test.d/bin/lib");
    EXPECT_TRUE(engine.isDirectory("test.d/bin/lib"));
    EXPECT_TRUE(engine.isDirectory("test.d/"));
    EXPECT_FALSE(engine.isDirectory("test.*"));
}

TEST(testCapioClEngine, testAddRemoveFile) {
    capiocl::Engine engine;
    engine.newFile("test.*");
    EXPECT_TRUE(engine.contains("test.*"));
    EXPECT_TRUE(engine.contains("test.txt"));
    engine.remove("test.*");
    EXPECT_FALSE(engine.contains("test.*"));
    engine.remove("data");
    EXPECT_FALSE(engine.contains("data"));
}

TEST(testCapioClEngine, testProducersConsumers) {
    capiocl::Engine engine;
    engine.newFile("test.*");

    std::string consumer = "consumer";
    std::string producer = "producer";

    engine.addConsumer("test.txt", consumer);
    engine.addProducer("test.txt.1", producer);

    EXPECT_TRUE(engine.isProducer("test.txt.1", producer));
    EXPECT_FALSE(engine.isProducer("test.txt.1", consumer));

    EXPECT_FALSE(engine.isConsumer("test.txt", producer));
    EXPECT_TRUE(engine.isConsumer("test.txt", consumer));

    engine.addConsumer("test.*", consumer);
    engine.addProducer("test.*", producer);

    EXPECT_TRUE(engine.isProducer("test.*", producer));
    EXPECT_FALSE(engine.isProducer("test.*", consumer));
    EXPECT_TRUE(engine.isConsumer("test.*", consumer));
    EXPECT_FALSE(engine.isConsumer("test.*", producer));

    engine.addProducer("test.k", producer);
    engine.addConsumer("test.k", consumer);
    EXPECT_TRUE(engine.isProducer("test.k", producer));
    EXPECT_TRUE(engine.isConsumer("test.k", consumer));

    EXPECT_TRUE(engine.isConsumer("test.txt.2", consumer));
    EXPECT_FALSE(engine.isProducer("test.txt.3", consumer));
    EXPECT_FALSE(engine.isConsumer("test.txt.4", producer));
    EXPECT_TRUE(engine.isProducer("test.txt.4", producer));

    EXPECT_EQ(engine.getProducers("myNewFile").size(), 0);
    EXPECT_EQ(engine.getProducers("test.k").size(), 1);
}

TEST(testCapioClEngine, testCommitCloseCount) {
    capiocl::Engine engine;
    engine.newFile("test.*");
    engine.setCommitRule("test.*", capiocl::COMMITTED_ON_CLOSE);
    engine.setCommitedCloseNumber("test.e", 100);

    EXPECT_EQ(engine.getCommitCloseCount("test.e"), 100);
    EXPECT_EQ(engine.getCommitCloseCount("test.d"), 0);

    engine.setCommitedCloseNumber("test.*", 30);
    EXPECT_EQ(engine.getCommitCloseCount("test.f"), 30);

    engine.setCommitRule("myFile", capiocl::COMMITTED_ON_FILE);
    EXPECT_TRUE(engine.getCommitRule("myFile") == capiocl::COMMITTED_ON_FILE);
}

TEST(testCapioClEngine, testStorageOptions) {
    capiocl::Engine engine;
    engine.newFile("A");
    engine.newFile("B");

    engine.setStoreFileInMemory("A");
    EXPECT_TRUE(engine.isStoredInMemory("A"));
    EXPECT_FALSE(engine.isStoredInMemory("B"));

    engine.setStoreFileInMemory("B");
    EXPECT_TRUE(engine.isStoredInMemory("A"));
    EXPECT_TRUE(engine.isStoredInMemory("B"));

    engine.setStoreFileInMemory("C");
    EXPECT_TRUE(engine.isStoredInMemory("C"));

    engine.newFile("D");
    EXPECT_FALSE(engine.isStoredInMemory("D"));

    engine.setAllStoreInMemory();
    EXPECT_TRUE(engine.isStoredInMemory("A"));
    EXPECT_TRUE(engine.isStoredInMemory("B"));
    EXPECT_TRUE(engine.isStoredInMemory("C"));
    EXPECT_TRUE(engine.isStoredInMemory("D"));

    EXPECT_EQ(engine.getFileToStoreInMemory().size(), 4);

    engine.setStoreFileInFileSystem("A");
    engine.setStoreFileInFileSystem("B");
    engine.setStoreFileInFileSystem("C");
    engine.setStoreFileInFileSystem("D");
    EXPECT_FALSE(engine.isStoredInMemory("A"));
    EXPECT_FALSE(engine.isStoredInMemory("B"));
    EXPECT_FALSE(engine.isStoredInMemory("C"));
    EXPECT_FALSE(engine.isStoredInMemory("D"));

    engine.setStoreFileInFileSystem("F");
    EXPECT_FALSE(engine.isStoredInMemory("F"));
}

TEST(testCapioClEngine, testHomeNode) {
    std::string nodename;
    nodename.reserve(1024);
    gethostname(nodename.data(), nodename.size());

    capiocl::Engine engine;
    engine.newFile("A");
    EXPECT_TRUE(engine.getHomeNode("A") == nodename);
    EXPECT_TRUE(engine.getHomeNode("B") == nodename);
}

TEST(testCapioClEngine, testInsertFileDependencies) {
    capiocl::Engine engine;

    engine.setFileDeps("myFile.txt", {});
    engine.setFileDeps("test.txt", {"a", "b", "c"});
    EXPECT_EQ(engine.getCommitOnFileDependencies("test.txt").size(), 3);
    EXPECT_TRUE(engine.getCommitOnFileDependencies("test.txt")[0] == "a");
    EXPECT_TRUE(engine.getCommitOnFileDependencies("test.txt")[1] == "b");
    EXPECT_TRUE(engine.getCommitOnFileDependencies("test.txt")[2] == "c");
}

TEST(testCapioClEngine, testEqualDifferentOperator) {
    capiocl::Engine engine1, engine2;

    engine1.newFile("A");
    engine2.newFile("A");

    engine1.setCommitRule("A", capiocl::COMMITTED_ON_CLOSE);
    engine2.setCommitRule("A", capiocl::COMMITTED_ON_TERMINATION);
    EXPECT_FALSE(engine1 == engine2);
    engine2.setCommitRule("A", engine1.getCommitRule("A"));

    engine1.setFireRule("A", capiocl::MODE_NO_UPDATE);
    engine2.setFireRule("A", capiocl::MODE_UPDATE);
    EXPECT_FALSE(engine1 == engine2);
    engine2.setFireRule("A", engine1.getFireRule("A"));

    engine1.setPermanent("A", true);
    engine2.setPermanent("A", false);
    EXPECT_FALSE(engine1 == engine2);
    engine2.setPermanent("A", engine1.isPermanent("A"));

    engine1.setExclude("A", true);
    engine2.setExclude("A", false);
    EXPECT_FALSE(engine1 == engine2);
    engine2.setExclude("A", engine1.isExcluded("A"));

    engine1.setFile("A");
    engine2.setDirectory("A");
    EXPECT_FALSE(engine1 == engine2);
    engine2.setFile("A");

    engine1.setCommitedCloseNumber("A", 10);
    engine2.setCommitedCloseNumber("A", 5);
    EXPECT_FALSE(engine1 == engine2);
    engine2.setCommitedCloseNumber("A", engine1.getCommitCloseCount("A"));

    engine1.setDirectoryFileCount("A", 10);
    engine2.setDirectoryFileCount("A", 5);
    EXPECT_FALSE(engine1 == engine2);
    engine2.setDirectoryFileCount("A", engine1.getDirectoryFileCount("A"));

    engine1.setStoreFileInFileSystem("A");
    engine2.setStoreFileInMemory("A");
    EXPECT_FALSE(engine1 == engine2);
    engine2.setStoreFileInFileSystem("A");

    engine2.newFile("C");
    EXPECT_FALSE(engine1 == engine2);
}

TEST(testCapioClEngine, testEqualDifferentOperatorDifferentPathSameSize) {
    capiocl::Engine engine1, engine2;
    engine1.newFile("A");
    engine2.newFile("B");
    EXPECT_FALSE(engine1 == engine2);
}

TEST(testCapioClEngine, testEqualDifferentProducers) {
    capiocl::Engine engine1, engine2;
    engine1.newFile("A");
    engine2.newFile("A");
    std::string stepA = "prod1";
    std::string stepB = "prod2";
    engine1.addProducer("A", stepA);
    EXPECT_FALSE(engine1 == engine2);
    engine2.addProducer("A", stepB);
    EXPECT_FALSE(engine1 == engine2);

    engine1.addProducer("A", stepB);
    engine2.addProducer("A", stepA);

    engine1.addConsumer("A", stepA);
    EXPECT_FALSE(engine1 == engine2);
    engine2.addConsumer("A", stepB);
    EXPECT_FALSE(engine1 == engine2);
}

TEST(testCapioClEngine, testFileDependenciesDifferences) {
    capiocl::Engine engine1, engine2;
    engine1.newFile("A");
    engine2.newFile("A");
    std::string depsA = "prod1";
    std::string depsB = "prod2";
    engine1.addFileDependency("A", depsA);
    EXPECT_FALSE(engine1 == engine2);
    engine2.addFileDependency("A", depsB);
    EXPECT_FALSE(engine1 == engine2);

    engine1.addFileDependency("A", depsB);
    EXPECT_FALSE(engine1 == engine2);
    engine2.addFileDependency("A", depsA);
    EXPECT_TRUE(engine1 == engine2);
}

TEST(testCapioSerializerParser, testSerializeParseCAPIOCLV1) {
    const std::filesystem::path path("./config.json");
    const std::string workflow_name = "demo";
    const std::string file_1_name = "file1.txt", file_2_name = "file2.txt",
                      file_3_name = "my_command_history.txt";
    std::string producer_name = "_first", consumer_name = "_last", intermediate_name = "_middle";

    capiocl::Engine engine;
    engine.addProducer(file_1_name, producer_name);
    engine.addConsumer(file_1_name, intermediate_name);
    engine.addProducer(file_2_name, intermediate_name);
    engine.addConsumer(file_2_name, consumer_name);
    engine.addConsumer(file_1_name, consumer_name);

    engine.setStoreFileInMemory(file_1_name);
    engine.setCommitRule(file_1_name, capiocl::COMMITTED_ON_CLOSE);
    engine.setCommitedCloseNumber(file_1_name, 3);
    engine.setFireRule(file_1_name, capiocl::MODE_UPDATE);
    engine.setPermanent(file_1_name, true);

    engine.setStoreFileInFileSystem(file_2_name);
    engine.setCommitRule(file_2_name, capiocl::COMMITTED_ON_TERMINATION);
    engine.setFireRule(file_1_name, capiocl::MODE_NO_UPDATE);

    engine.addProducer(file_3_name, producer_name);
    engine.addProducer(file_3_name, consumer_name);
    engine.addProducer(file_3_name, intermediate_name);
    engine.setExclude(file_3_name, true);
    engine.print();

    capiocl::Serializer::dump(engine, workflow_name, path);

    std::filesystem::path resolve = "";
    auto [wf_name, new_engine]    = capiocl::Parser::parse(path, resolve);

    EXPECT_TRUE(wf_name == workflow_name);
    capiocl::print_message("", "");
    EXPECT_TRUE(engine == *new_engine);

    auto [wf_name1, new_engine1] = capiocl::Parser::parse(path, resolve, true);
    EXPECT_EQ(new_engine1->getFileToStoreInMemory().size(), 3);

    std::filesystem::remove(path);
}

TEST(testCapioSerializerParser, testSerializeParseCAPIOCLV1NcloseNfiles) {
    const std::filesystem::path path("./config.json");
    const std::string workflow_name = "demo";
    const std::string file_1_name   = "file1.txt";
    std::string producer_name = "_first", consumer_name = "_last";

    capiocl::Engine engine;

    engine.setDirectory(file_1_name);
    engine.setDirectoryFileCount(file_1_name, 10);
    engine.addProducer(file_1_name, producer_name);
    engine.addConsumer(file_1_name, consumer_name);

    capiocl::Serializer::dump(engine, workflow_name, path);

    std::filesystem::path resolve = "";
    auto [wf_name, new_engine]    = capiocl::Parser::parse(path, resolve);

    EXPECT_TRUE(wf_name == workflow_name);
    capiocl::print_message("", "");
    EXPECT_TRUE(engine == *new_engine);

    std::filesystem::remove(path);
}

template <typename T> std::string demangled_name(const T &obj) {
    int status;
    const char *mangled = typeid(obj).name();
    std::unique_ptr<char, void (*)(void *)> demangled(
        abi::__cxa_demangle(mangled, nullptr, nullptr, &status), std::free);
    return status == 0 ? demangled.get() : mangled;
}

TEST(testCapioSerializerParser, testParserException) {
    std::filesystem::path JSON_DIR = "/tmp/capio_cl_jsons";
    capiocl::print_message(capiocl::CLI_LEVEL_INFO, "Loading jsons from " + JSON_DIR.string());
    bool exception_catched = false;

    std::vector<std::filesystem::path> test_filenames = {
        "",
        "ANonExistingFile",
        JSON_DIR / "V1_test1.json",
        JSON_DIR / "V1_test2.json",
        JSON_DIR / "V1_test3.json",
        JSON_DIR / "V1_test4.json",
        JSON_DIR / "V1_test5.json",
        JSON_DIR / "V1_test6.json",
        JSON_DIR / "V1_test7.json",
        JSON_DIR / "V1_test8.json",
        JSON_DIR / "V1_test9.json",
        JSON_DIR / "V1_test10.json",
        JSON_DIR / "V1_test11.json",
        JSON_DIR / "V1_test12.json",
        JSON_DIR / "V1_test13.json",
        JSON_DIR / "V1_test14.json",
        JSON_DIR / "V1_test15.json",
        JSON_DIR / "V1_test16.json",
        JSON_DIR / "V1_test17.json",
        JSON_DIR / "V1_test18.json",
        JSON_DIR / "V1_test19.json",
        JSON_DIR / "V1_test20.json",
        JSON_DIR / "V1_test21.json",
    };

    for (const auto &test : test_filenames) {
        exception_catched = false;
        try {
            capiocl::print_message(capiocl::CLI_LEVEL_WARNING, "Testing on file " + test.string());
            auto [wf_name, engine] = capiocl::Parser::parse(test);
        } catch (std::exception &e) {
            exception_catched = true;
            EXPECT_TRUE(demangled_name(e) == "capiocl::ParserException");
            EXPECT_GT(std::string(e.what()).size(), 0);
        }
        EXPECT_TRUE(exception_catched);
        capiocl::print_message(capiocl::CLI_LEVEL_WARNING);
    }
}