#ifndef CAPIO_CL_ENGINE_HPP
#define CAPIO_CL_ENGINE_HPP

#define ENGINE_SUITE_NAME testEngine

TEST(ENGINE_SUITE_NAME, testInstantiation) {
    capiocl::engine::Engine engine;
    EXPECT_EQ(engine.size(), 0);
    engine.print();
}

TEST(ENGINE_SUITE_NAME, testSetGetWfName) {
    capiocl::engine::Engine engine;
    EXPECT_TRUE(engine.getWorkflowName() == capiocl::CAPIO_CL_DEFAULT_WF_NAME);
    engine.setWorkflowName("test");
    EXPECT_TRUE(engine.getWorkflowName() == "test");
}

TEST(ENGINE_SUITE_NAME, testSetGetWfNameFromEnv) {
    setenv("WORKFLOW_NAME", "my_custom_wf_name", 1);
    capiocl::engine::Engine engine;
    EXPECT_TRUE(engine.getWorkflowName() == "my_custom_wf_name");
}

TEST(ENGINE_SUITE_NAME, testAddFileDefault) {
    capiocl::engine::Engine engine;
    EXPECT_EQ(engine.size(), 0);
    engine.newFile("test.dat");
    EXPECT_EQ(engine.size(), 1);
    EXPECT_EQ(engine.getCommitRule("test.dat"), capiocl::commitRules::ON_TERMINATION);
    EXPECT_EQ(engine.getFireRule("test.dat"), capiocl::fireRules::UPDATE);
    EXPECT_TRUE(engine.getConsumers("test.dat").empty());
    EXPECT_TRUE(engine.getProducers("test.dat").empty());
    EXPECT_FALSE(engine.isPermanent("test.dat"));
    EXPECT_FALSE(engine.isExcluded("test.dat"));
    EXPECT_TRUE(engine.isFile("test.dat"));
    EXPECT_FALSE(engine.isDirectory("test.dat"));
    EXPECT_EQ(engine.getDirectoryFileCount("test.dat"), 0);
    EXPECT_FALSE(engine.isStoredInMemory("test.dat"));
}

TEST(ENGINE_SUITE_NAME, testAddFileDefaultGlob) {
    capiocl::engine::Engine engine;
    EXPECT_EQ(engine.size(), 0);
    engine.newFile("test.*");
    EXPECT_EQ(engine.size(), 1);
    EXPECT_EQ(engine.getCommitRule("test.dat"), capiocl::commitRules::ON_TERMINATION);
    EXPECT_EQ(engine.getFireRule("test.dat"), capiocl::fireRules::UPDATE);
    EXPECT_TRUE(engine.getConsumers("test.dat").empty());
    EXPECT_TRUE(engine.getProducers("test.dat").empty());
    EXPECT_FALSE(engine.isPermanent("test.dat"));
    EXPECT_FALSE(engine.isExcluded("test.dat"));
    EXPECT_TRUE(engine.isFile("test.dat"));
    EXPECT_FALSE(engine.isDirectory("test.dat"));
    EXPECT_EQ(engine.getDirectoryFileCount("test.dat"), 0);
    EXPECT_FALSE(engine.isStoredInMemory("test.dat"));
}

TEST(ENGINE_SUITE_NAME, testAddFileDefaultGlobQuestion) {
    capiocl::engine::Engine engine;
    EXPECT_EQ(engine.size(), 0);
    engine.newFile("test.?");
    EXPECT_EQ(engine.size(), 1);
    EXPECT_EQ(engine.getCommitRule("test.1"), capiocl::commitRules::ON_TERMINATION);
    EXPECT_EQ(engine.getFireRule("test.1"), capiocl::fireRules::UPDATE);
    EXPECT_TRUE(engine.getConsumers("test.1").empty());
    EXPECT_TRUE(engine.getProducers("test.1").empty());
    EXPECT_FALSE(engine.isPermanent("test.1"));
    EXPECT_FALSE(engine.isExcluded("test.1"));
    EXPECT_TRUE(engine.isFile("test.1"));
    EXPECT_FALSE(engine.isDirectory("test.1"));
    EXPECT_EQ(engine.getDirectoryFileCount("test.1"), 0);
    EXPECT_FALSE(engine.isStoredInMemory("test.1"));
}

TEST(ENGINE_SUITE_NAME, testAddFileManually) {
    capiocl::engine::Engine engine;
    EXPECT_EQ(engine.size(), 0);
    std::filesystem::path path = "test.dat";
    std::vector<std::string> producers, consumers;
    std::vector<std::filesystem::path> file_dependencies;

    engine.add(path, producers, consumers, capiocl::commitRules::ON_TERMINATION,
               capiocl::fireRules::UPDATE, false, false, file_dependencies);
    EXPECT_EQ(engine.size(), 1);
    EXPECT_EQ(engine.getCommitRule("test.dat"), capiocl::commitRules::ON_TERMINATION);
    EXPECT_EQ(engine.getFireRule("test.dat"), capiocl::fireRules::UPDATE);
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

TEST(ENGINE_SUITE_NAME, testAddFileManuallyGlob) {
    capiocl::engine::Engine engine;
    EXPECT_EQ(engine.size(), 0);
    std::filesystem::path path = "test.*";
    std::vector<std::string> producers, consumers;
    std::vector<std::filesystem::path> file_dependencies;

    engine.add(path, producers, consumers, capiocl::commitRules::ON_TERMINATION,
               capiocl::fireRules::UPDATE, false, false, file_dependencies);
    EXPECT_EQ(engine.size(), 1);
    EXPECT_EQ(engine.getCommitRule("test.dat"), capiocl::commitRules::ON_TERMINATION);
    EXPECT_EQ(engine.getFireRule("test.dat"), capiocl::fireRules::UPDATE);
    EXPECT_TRUE(engine.getConsumers("test.dat").empty());
    EXPECT_TRUE(engine.getProducers("test.dat").empty());
    EXPECT_FALSE(engine.isPermanent("test.dat"));
    EXPECT_FALSE(engine.isExcluded("test.dat"));
    EXPECT_TRUE(engine.isFile("test.dat"));
    EXPECT_FALSE(engine.isDirectory("test.dat"));
    EXPECT_EQ(engine.getDirectoryFileCount("test.dat"), 0);
    EXPECT_FALSE(engine.isStoredInMemory("test.dat"));
}

TEST(ENGINE_SUITE_NAME, testAddFileManuallyQuestion) {
    capiocl::engine::Engine engine;
    EXPECT_EQ(engine.size(), 0);
    std::filesystem::path path = "test.?";
    std::vector<std::string> producers, consumers;
    std::vector<std::filesystem::path> file_dependencies;

    engine.add(path, producers, consumers, capiocl::commitRules::ON_CLOSE,
               capiocl::fireRules::NO_UPDATE, false, false, file_dependencies);
    engine.setDirectory("test.?");
    engine.setDirectoryFileCount("test.?", 10);

    EXPECT_EQ(engine.size(), 1);
    EXPECT_FALSE(engine.getCommitRule("test.dat") == capiocl::commitRules::ON_CLOSE);
    EXPECT_FALSE(engine.getFireRule("test.dat") == capiocl::fireRules::NO_UPDATE);
    EXPECT_EQ(engine.getCommitRule("test.1"), capiocl::commitRules::ON_CLOSE);
    EXPECT_EQ(engine.getFireRule("test.1"), capiocl::fireRules::NO_UPDATE);
    EXPECT_EQ(engine.getCommitRule("test.2"), capiocl::commitRules::ON_CLOSE);
    EXPECT_EQ(engine.getFireRule("test.2"), capiocl::fireRules::NO_UPDATE);
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

TEST(ENGINE_SUITE_NAME, testAddFileManuallyGlobExplcit) {
    capiocl::engine::Engine engine;
    EXPECT_EQ(engine.size(), 0);
    std::filesystem::path path = "test.[abc][abc][abc]";
    std::vector<std::string> producers, consumers;
    std::vector<std::filesystem::path> file_dependencies;

    engine.add(path, producers, consumers, capiocl::commitRules::ON_CLOSE,
               capiocl::fireRules::NO_UPDATE, false, false, file_dependencies);
    engine.setDirectory("test.[abc][abc][abc]");
    engine.setDirectoryFileCount("test.[abc][abc][abc]", 10);

    EXPECT_EQ(engine.size(), 1);
    EXPECT_FALSE(engine.getCommitRule("test.dat") == capiocl::commitRules::ON_CLOSE);
    EXPECT_FALSE(engine.getFireRule("test.dat") == capiocl::fireRules::NO_UPDATE);
    EXPECT_TRUE(engine.getCommitRule("test.abc") == capiocl::commitRules::ON_CLOSE);
    EXPECT_TRUE(engine.getFireRule("test.aaa") == capiocl::fireRules::NO_UPDATE);
    EXPECT_EQ(engine.getCommitRule("test.cab"), capiocl::commitRules::ON_CLOSE);
    EXPECT_EQ(engine.getFireRule("test.bac"), capiocl::fireRules::NO_UPDATE);
    EXPECT_EQ(engine.getCommitRule("test.ccc"), capiocl::commitRules::ON_CLOSE);
    EXPECT_EQ(engine.getFireRule("test.aaa"), capiocl::fireRules::NO_UPDATE);
    EXPECT_TRUE(engine.isDirectory("test.bbb"));
    EXPECT_NE(engine.getDirectoryFileCount("test.3"), 10);
}

TEST(ENGINE_SUITE_NAME, testProducerConsumersFileDependencies) {
    capiocl::engine::Engine engine;
    EXPECT_EQ(engine.size(), 0);
    std::vector<std::string> producers = {"A", "B"}, consumers = {"C", "D"};
    std::vector<std::filesystem::path> file_dependencies = {"E", "F"};

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
    EXPECT_TRUE(engine.getCommitRule("myFile.txt") == capiocl::commitRules::ON_FILE);
    EXPECT_EQ(engine.getCommitOnFileDependencies("myFile.txt").size(), 1);
    EXPECT_TRUE(engine.getCommitOnFileDependencies("myFile.txt")[0] == file_dependencies[0]);
}

TEST(ENGINE_SUITE_NAME, testProducerConsumersFileDependenciesGlob) {
    capiocl::engine::Engine engine;
    EXPECT_EQ(engine.size(), 0);
    std::vector<std::string> producers = {"A", "B"}, consumers = {"C", "D"};
    std::vector<std::filesystem::path> file_dependencies = {"E", "F"};

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

TEST(ENGINE_SUITE_NAME, testCommitFirePermanentExcludeOnGlobs) {
    capiocl::engine::Engine engine;
    engine.newFile("test.*");
    engine.setFireRule("test.*", capiocl::fireRules::NO_UPDATE);

    EXPECT_TRUE(engine.isFirable("test.a"));
    EXPECT_FALSE(engine.isFirable("testb"));

    engine.setCommitRule("testb", capiocl::commitRules::ON_FILE);
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

    engine.setFireRule("test.c", capiocl::fireRules::NO_UPDATE);
    EXPECT_TRUE(engine.isFirable("test.c"));

    engine.setCommitedCloseNumber("test.e", 100);
}

TEST(ENGINE_SUITE_NAME, testIsFileIsDirectoryGlob) {
    capiocl::engine::Engine engine;
    engine.newFile("test.*");
    engine.setDirectory("test.d/");
    engine.setDirectory("test.d/bin/lib");
    EXPECT_TRUE(engine.isDirectory("test.d/bin/lib"));
    EXPECT_TRUE(engine.isDirectory("test.d/"));
    EXPECT_FALSE(engine.isDirectory("test.*"));
}

TEST(ENGINE_SUITE_NAME, testAddRemoveFile) {
    capiocl::engine::Engine engine;
    engine.newFile("test.*");
    EXPECT_TRUE(engine.contains("test.*"));
    EXPECT_TRUE(engine.contains("test.txt"));
    engine.remove("test.*");
    EXPECT_FALSE(engine.contains("test.*"));
    engine.remove("data");
    EXPECT_FALSE(engine.contains("data"));
}

TEST(ENGINE_SUITE_NAME, testProducersConsumers) {
    capiocl::engine::Engine engine;
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

TEST(ENGINE_SUITE_NAME, testCommitCloseCount) {
    capiocl::engine::Engine engine;
    engine.newFile("test.*");
    engine.setCommitRule("test.*", capiocl::commitRules::ON_CLOSE);
    engine.setCommitedCloseNumber("test.e", 100);

    EXPECT_EQ(engine.getCommitCloseCount("test.e"), 100);
    EXPECT_EQ(engine.getCommitCloseCount("test.d"), 0);

    engine.setCommitedCloseNumber("test.*", 30);
    EXPECT_EQ(engine.getCommitCloseCount("test.f"), 30);

    engine.setCommitRule("myFile", capiocl::commitRules::ON_FILE);
    EXPECT_TRUE(engine.getCommitRule("myFile") == capiocl::commitRules::ON_FILE);
}

TEST(ENGINE_SUITE_NAME, testStorageOptions) {
    capiocl::engine::Engine engine;
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

TEST(ENGINE_SUITE_NAME, testInsertFileDependencies) {
    capiocl::engine::Engine engine;

    engine.setFileDeps("myFile.txt", {});
    engine.setFileDeps("test.txt", {"a", "b", "c"});
    EXPECT_EQ(engine.getCommitOnFileDependencies("test.txt").size(), 3);
    EXPECT_TRUE(engine.getCommitOnFileDependencies("test.txt")[0] == "a");
    EXPECT_TRUE(engine.getCommitOnFileDependencies("test.txt")[1] == "b");
    EXPECT_TRUE(engine.getCommitOnFileDependencies("test.txt")[2] == "c");
}

TEST(ENGINE_SUITE_NAME, testComputeDirectoryFileCount) {
    capiocl::engine::Engine e;
    e.newFile("a");
    e.newFile("a/b");
    e.newFile("a/b/c");
    e.newFile("a/b/d");
    e.newFile("a/e");
    e.newFile("a/b/c/f");

    EXPECT_EQ(e.getDirectoryFileCount("a"), 2);
    EXPECT_EQ(e.getDirectoryFileCount("a/b"), 2);
    EXPECT_EQ(e.getDirectoryFileCount("a/e"), 0);
    EXPECT_EQ(e.getDirectoryFileCount("a/b/c"), 1);
    EXPECT_EQ(e.getDirectoryFileCount("a/b/d"), 0);
    EXPECT_EQ(e.getDirectoryFileCount("a/b/c/f"), 0);

    EXPECT_TRUE(e.isDirectory("a"));
    EXPECT_TRUE(e.isDirectory("a/b"));
    EXPECT_FALSE(e.isDirectory("a/e"));

    e.newFile("a/e/k");
    EXPECT_TRUE(e.isDirectory("a/e"));
    EXPECT_EQ(e.getDirectoryFileCount("a/e"), 1);

    e.setDirectoryFileCount("a/b", 10);
    e.newFile("a/b/r");
    EXPECT_EQ(e.getDirectoryFileCount("a/b"), 10);
    e.newFile("a/b/r/f");
    EXPECT_EQ(e.getDirectoryFileCount("a/b/r"), 1);
}

TEST(ENGINE_SUITE_NAME, testEqualDifferentOperator) {
    capiocl::engine::Engine engine1, engine2;

    engine1.newFile("A");
    engine2.newFile("A");

    engine1.setCommitRule("A", capiocl::commitRules::ON_CLOSE);
    engine2.setCommitRule("A", capiocl::commitRules::ON_TERMINATION);
    EXPECT_FALSE(engine1 == engine2);
    engine2.setCommitRule("A", engine1.getCommitRule("A"));

    engine1.setFireRule("A", capiocl::fireRules::NO_UPDATE);
    engine2.setFireRule("A", capiocl::fireRules::UPDATE);
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

TEST(ENGINE_SUITE_NAME, testEqualDifferentOperatorDifferentPathSameSize) {
    capiocl::engine::Engine engine1, engine2;
    engine1.newFile("A");
    engine2.newFile("B");
    EXPECT_FALSE(engine1 == engine2);
}

TEST(ENGINE_SUITE_NAME, testEqualDifferentProducers) {
    capiocl::engine::Engine engine1, engine2;
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

TEST(ENGINE_SUITE_NAME, testStoreAllInMemory) {
    capiocl::engine::Engine engine;
    engine.setAllStoreInMemory();

    engine.newFile("A*");
    EXPECT_TRUE(engine.isStoredInMemory("A*"));
    engine.newFile("A.B");
    EXPECT_TRUE(engine.isStoredInMemory("A*"));
}

TEST(ENGINE_SUITE_NAME, testFileDependenciesDifferences) {
    capiocl::engine::Engine engine1, engine2;
    engine1.newFile("A");
    engine2.newFile("A");
    std::filesystem::path depsA = "prod1";
    std::filesystem::path depsB = "prod2";
    engine1.addFileDependency("A", depsA);
    EXPECT_FALSE(engine1 == engine2);
    engine2.addFileDependency("A", depsB);
    EXPECT_FALSE(engine1 == engine2);

    engine1.addFileDependency("A", depsB);
    EXPECT_FALSE(engine1 == engine2);
    engine2.addFileDependency("A", depsA);
    EXPECT_TRUE(engine1 == engine2);
}

TEST(ENGINE_SUITE_NAME, testOnEmptyPath) {
    capiocl::engine::Engine engine;

    std::vector<std::string> empty1, empty2;
    std::filesystem::path empty_file_name;
    std::string producer_name_1 = "producer", producer_name_2 = "producer";
    std::vector<std::filesystem::path> file_dependencies;

    engine.newFile(empty_file_name);
    EXPECT_EQ(engine.size(), 0);
    engine.add(empty_file_name, empty1, empty2, capiocl::commitRules::ON_TERMINATION,
               capiocl::fireRules::UPDATE, true, true, file_dependencies);
    EXPECT_EQ(engine.size(), 0);
    EXPECT_EQ(engine.getDirectoryFileCount(empty_file_name), 0);

    engine.addProducer(empty_file_name, producer_name_1);
    engine.addConsumer(empty_file_name, producer_name_1);
    engine.addFileDependency(empty_file_name, empty_file_name);
    engine.setCommitRule(empty_file_name, capiocl::commitRules::ON_TERMINATION);
    engine.setFireRule(empty_file_name, capiocl::fireRules::UPDATE);

    EXPECT_TRUE(engine.getCommitRule(empty_file_name) == capiocl::commitRules::ON_TERMINATION);
    EXPECT_TRUE(engine.getFireRule(empty_file_name) == capiocl::fireRules::NO_UPDATE);
    EXPECT_TRUE(engine.isFirable(empty_file_name));

    engine.setPermanent(empty_file_name, false);
    EXPECT_TRUE(engine.isPermanent(empty_file_name));

    engine.setExclude(empty_file_name, false);
    EXPECT_TRUE(engine.isExcluded(empty_file_name));

    engine.setDirectory(empty_file_name);
    EXPECT_TRUE(engine.isDirectory(empty_file_name));
    engine.setFile(empty_file_name);
    EXPECT_TRUE(engine.isFile(empty_file_name));

    engine.setCommitedCloseNumber(empty_file_name, 10);
    engine.setDirectoryFileCount(empty_file_name, 10);
    EXPECT_EQ(engine.getDirectoryFileCount(empty_file_name), 0);
    EXPECT_EQ(engine.getCommitCloseCount(empty_file_name), 0);
    EXPECT_TRUE(engine.isConsumer(empty_file_name, producer_name_2));
    EXPECT_TRUE(engine.isProducer(empty_file_name, producer_name_1));
    EXPECT_TRUE(engine.getProducers(empty_file_name).empty());

    engine.setFileDeps(empty_file_name, {});

    engine.setStoreFileInMemory(empty_file_name);
    engine.setStoreFileInFileSystem(empty_file_name);
    EXPECT_TRUE(engine.isStoredInMemory(empty_file_name));

    engine.setExclude(empty_file_name, false);
    EXPECT_TRUE(engine.isExcluded(empty_file_name));
}

TEST(ENGINE_SUITE_NAME, TestGetPaths) {
    capiocl::engine::Engine engine;
    engine.newFile("A");
    engine.newFile("B");
    engine.newFile("/tmp");

    std::vector<std::string> paths = engine.getPaths();

    EXPECT_TRUE(!paths.empty());
    EXPECT_EQ(paths.size(), 3);
    EXPECT_TRUE(std::find(paths.begin(), paths.end(), "A") != paths.end());
    EXPECT_TRUE(std::find(paths.begin(), paths.end(), "B") != paths.end());
    EXPECT_TRUE(std::find(paths.begin(), paths.end(), "/tmp") != paths.end());
}

TEST(ENGINE_SUITE_NAME, TestSubfolderMatching) {
    capiocl::engine::Engine engine;
    std::string producer_name = "producer";
    engine.addProducer("/a/*", producer_name);
    EXPECT_TRUE(engine.isProducer("/a/b", producer_name));
    EXPECT_TRUE(engine.isProducer("/a/b/", producer_name));
    EXPECT_TRUE(engine.isProducer("/a/b/c", producer_name));
}

TEST(ENGINE_SUITE_NAME, TestInheritanceFromParentPaths) {
    capiocl::engine::Engine engine;

    engine.newFile("/test/*");
    engine.setCommitRule("/test/*", capiocl::commitRules::ON_CLOSE);
    engine.setFireRule("/test/*", capiocl::fireRules::NO_UPDATE);

    engine.newFile("/test/a/b/c/d");
    EXPECT_TRUE(engine.getCommitRule("/test/a/b/c/d") == capiocl::commitRules::ON_CLOSE);
    EXPECT_TRUE(engine.getFireRule("/test/a/b/c/d") == capiocl::fireRules::NO_UPDATE);

    engine.setCommitRule("/test/*", capiocl::commitRules::ON_TERMINATION);
    engine.setFireRule("/test/*", capiocl::fireRules::UPDATE);
    EXPECT_TRUE(engine.getCommitRule("/test/a/b/c/d") == capiocl::commitRules::ON_CLOSE);
    EXPECT_TRUE(engine.getFireRule("/test/a/b/c/d") == capiocl::fireRules::NO_UPDATE);
}

#endif // CAPIO_CL_ENGINE_HPP