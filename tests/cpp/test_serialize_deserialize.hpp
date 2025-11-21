#ifndef CAPIO_CL_TEST_SERIALIZE_DESERIALIZE_HPP
#define CAPIO_CL_TEST_SERIALIZE_DESERIALIZE_HPP

#define SERIALIZE_DESERIALIZE_SUITE_NAME TestSerializeAndDeserialize

TEST(SERIALIZE_DESERIALIZE_SUITE_NAME, testSerializeParseCAPIOCLV1) {
    for (const auto &_cl_version : CAPIO_CL_AVAIL_VERSIONS) {
        const std::filesystem::path path("./config.json");
        const std::string workflow_name = "demo";
        const std::string file_1_name = "file1.txt", file_2_name = "file2.txt",
                          file_3_name = "my_command_history.txt", file_4_name = "/tmp";
        std::string producer_name = "_first", consumer_name = "_last",
                    intermediate_name = "_middle";

        capiocl::engine::Engine engine;
        engine.setWorkflowName(workflow_name);
        engine.addProducer(file_1_name, producer_name);
        engine.addConsumer(file_1_name, intermediate_name);
        engine.addProducer(file_2_name, intermediate_name);
        engine.addConsumer(file_2_name, consumer_name);
        engine.addConsumer(file_1_name, consumer_name);

        engine.setStoreFileInMemory(file_1_name);
        engine.setCommitRule(file_1_name, capiocl::commitRules::ON_CLOSE);
        engine.setCommitedCloseNumber(file_1_name, 3);
        engine.setFireRule(file_1_name, capiocl::fireRules::UPDATE);
        engine.setPermanent(file_1_name, true);

        engine.setStoreFileInFileSystem(file_2_name);
        engine.setCommitRule(file_2_name, capiocl::commitRules::ON_TERMINATION);
        engine.setFireRule(file_1_name, capiocl::fireRules::NO_UPDATE);

        engine.addProducer(file_3_name, producer_name);
        engine.addProducer(file_3_name, consumer_name);
        engine.addProducer(file_3_name, intermediate_name);
        engine.setExclude(file_3_name, true);

        engine.setCommitRule(file_4_name, capiocl::commitRules::ON_N_FILES);
        engine.setFireRule(file_4_name, capiocl::fireRules::NO_UPDATE);
        engine.setDirectoryFileCount(file_4_name, 10);
        engine.addProducer(file_4_name, intermediate_name);

        engine.print();

        capiocl::serializer::Serializer::dump(engine, path, _cl_version);

        std::filesystem::path resolve = "";
        auto new_engine               = capiocl::parser::Parser::parse(path, resolve);

        EXPECT_TRUE(new_engine->getWorkflowName() == workflow_name);
        capiocl::printer::print("", "");
        EXPECT_TRUE(engine == *new_engine);

        auto new_engine1 = capiocl::parser::Parser::parse(path, resolve, true);
        EXPECT_EQ(new_engine1->getFileToStoreInMemory().size(), engine.size());

        std::filesystem::remove(path);
    }
}

TEST(SERIALIZE_DESERIALIZE_SUITE_NAME, testSerializeParseCAPIOCLV1NcloseNfiles) {
    for (const auto &_cl_version : CAPIO_CL_AVAIL_VERSIONS) {

        const std::filesystem::path path("./config.json");
        const std::string workflow_name = "demo";
        const std::string file_1_name   = "file1.txt";
        std::string producer_name = "_first", consumer_name = "_last";

        capiocl::engine::Engine engine;
        engine.setWorkflowName(workflow_name);

        engine.setDirectory(file_1_name);
        engine.setDirectoryFileCount(file_1_name, 10);
        engine.addProducer(file_1_name, producer_name);
        engine.addConsumer(file_1_name, consumer_name);

        capiocl::serializer::Serializer::dump(engine, path, _cl_version);

        std::filesystem::path resolve = "";
        auto new_engine               = capiocl::parser::Parser::parse(path, resolve);

        EXPECT_TRUE(new_engine->getWorkflowName() == workflow_name);
        capiocl::printer::print("", "");
        EXPECT_TRUE(engine == *new_engine);

        std::filesystem::remove(path);
    }
}

TEST(SERIALIZE_DESERIALIZE_SUITE_NAME, testSerializeParseCAPIOCLV1FileDeps) {
    for (const auto &_cl_version : CAPIO_CL_AVAIL_VERSIONS) {
        const std::filesystem::path path("./config.json");
        const std::string workflow_name = "demo";
        const std::string file_1_name = "file1.txt", file_2_name = "file2.txt",
                          file_3_name = "file3.txt";
        std::string producer_name = "_first", consumer_name = "_last";

        capiocl::engine::Engine engine;
        engine.setWorkflowName(workflow_name);

        engine.newFile(file_1_name);
        engine.newFile(file_2_name);
        engine.addProducer(file_1_name, producer_name);
        engine.addProducer(file_2_name, producer_name);

        engine.newFile(file_3_name);
        engine.addConsumer(file_3_name, consumer_name);
        engine.addProducer(file_3_name, producer_name);
        engine.setCommitRule(file_3_name, capiocl::commitRules::ON_FILE);
        engine.setFileDeps(file_3_name, {file_1_name, file_2_name});

        engine.print();
        capiocl::serializer::Serializer::dump(engine, path, _cl_version);

        std::filesystem::path resolve = "";
        auto new_engine               = capiocl::parser::Parser::parse(path, resolve);

        EXPECT_TRUE(new_engine->getWorkflowName() == workflow_name);
        capiocl::printer::print("", "");
        EXPECT_TRUE(engine == *new_engine);

        std::filesystem::remove(path);
    }
}

TEST(SERIALIZE_DESERIALIZE_SUITE_NAME, testSerializeCommitOnCloseCountNoCommitRule) {
    for (const auto &_cl_version : CAPIO_CL_AVAIL_VERSIONS) {
        const std::filesystem::path path("./config.json");
        const std::string workflow_name = "demo";
        const std::string file_1_name   = "file1.txt";
        std::string producer_name = "_first", consumer_name = "_last";

        capiocl::engine::Engine engine;
        engine.setWorkflowName(workflow_name);

        engine.newFile(file_1_name);
        engine.addProducer(file_1_name, producer_name);
        engine.setCommitRule(file_1_name, capiocl::commitRules::ON_TERMINATION);
        engine.setCommitedCloseNumber(file_1_name, 10);

        engine.print();
        capiocl::serializer::Serializer::dump(engine, path, _cl_version);

        std::filesystem::path resolve = "";
        auto new_engine               = capiocl::parser::Parser::parse(path, resolve);

        EXPECT_TRUE(new_engine->getWorkflowName() == workflow_name);
        EXPECT_FALSE(engine == *new_engine);
        capiocl::printer::print("", "");
        engine.setCommitRule(file_1_name, capiocl::commitRules::ON_CLOSE);
        EXPECT_TRUE(engine == *new_engine);

        std::filesystem::remove(path);
    }
}

TEST(SERIALIZE_DESERIALIZE_SUITE_NAME, testParserResolveAbsolute) {
    for (const auto &_cl_version : CAPIO_CL_AVAIL_VERSIONS) {
        const std::filesystem::path json_path("/tmp/capio_cl_jsons/V" + _cl_version +
                                              "/test0.json");
        auto engine = capiocl::parser::Parser::parse(json_path, "/tmp");
        EXPECT_TRUE(engine->getWorkflowName() == "test");
        EXPECT_TRUE(engine->contains("/tmp/file"));
        EXPECT_TRUE(engine->contains("/tmp/file1"));
        EXPECT_TRUE(engine->contains("/tmp/file2"));
        EXPECT_TRUE(engine->contains("/tmp/file3"));
    }
}

TEST(SERIALIZE_DESERIALIZE_SUITE_NAME, testNoStorageSection) {

    for (const auto &_cl_version : CAPIO_CL_AVAIL_VERSIONS) {
        const std::filesystem::path json_path("/tmp/capio_cl_jsons/V" + _cl_version +
                                              "/test24.json");
        auto engine = capiocl::parser::Parser::parse(json_path, "/tmp");
        EXPECT_TRUE(engine->getWorkflowName() == "test");
        EXPECT_TRUE(engine->contains("/tmp/file"));
        EXPECT_TRUE(engine->contains("/tmp/file1"));
    }
}
#endif // CAPIO_CL_TEST_SERIALIZE_DESERIALIZE_HPP