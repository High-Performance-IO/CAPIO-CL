#ifndef CAPIO_CL_EXCEPTIONS_HPP
#define CAPIO_CL_EXCEPTIONS_HPP

#define EXCEPTION_SUITE_NAME TestThrowExceptions
#include "include/serializer.h"

TEST(EXCEPTION_SUITE_NAME, testFailedDump) {
    const std::filesystem::path json_path("/tmp/capio_cl_jsons/V1_test24.json");
    auto engine            = capiocl::parser::Parser::parse(json_path, "/tmp");
    bool exception_catched = false;

    try {
        capiocl::serializer::Serializer::dump(*engine, "/");
    } catch (std::exception &e) {
        exception_catched = true;
        auto demangled    = demangled_name(e);
        capiocl::printer::print(capiocl::printer::CLI_LEVEL_INFO,
                                "Caught exception of type =" + demangled);
        EXPECT_TRUE(demangled == "capiocl::serializer::SerializerException");
        EXPECT_GT(std::string(e.what()).size(), 0);
    }

    EXPECT_TRUE(exception_catched);
}

TEST(EXCEPTION_SUITE_NAME, testFailedserializeVersion) {
    const std::filesystem::path json_path("/tmp/capio_cl_jsons/V1_test24.json");
    auto engine            = capiocl::parser::Parser::parse(json_path, "/tmp");
    bool exception_catched = false;

    try {
        capiocl::serializer::Serializer::dump(*engine, "test.json", "1234.5678");
    } catch (std::exception &e) {
        exception_catched = true;
        auto demangled    = demangled_name(e);
        capiocl::printer::print(capiocl::printer::CLI_LEVEL_INFO,
                                "Caught exception of type =" + demangled);
        EXPECT_TRUE(demangled == "capiocl::serializer::SerializerException");
        EXPECT_GT(std::string(e.what()).size(), 0);
    }

    EXPECT_TRUE(exception_catched);
}

TEST(EXCEPTION_SUITE_NAME, testParserException) {
    std::filesystem::path JSON_DIR    = "/tmp/capio_cl_jsons/";
    std::vector<std::string> VERSIONS = {"V1", "V1_1"};
    capiocl::printer::print(capiocl::printer::CLI_LEVEL_INFO,
                            "Loading jsons from " + JSON_DIR.string());

    std::vector<std::filesystem::path> test_filenames = {
        "",
        "ANonExistingFile",
        "test1.json",
        "test2.json",
        "test3.json",
        "test4.json",
        "test5.json",
        "test6.json",
        "test7.json",
        "test8.json",
        "test9.json",
        "test10.json",
        "test11.json",
        "test12.json",
        "test13.json",
        "test14.json",
        "test15.json",
        "test16.json",
        "test17.json",
        "test18.json",
        "test19.json",
        "test20.json",
        "test21.json",
        "test22.json",
        "test23.json",
        "test25.json",
    };
    for (const auto &version : VERSIONS) {
        for (const auto &test : test_filenames) {
            const auto test_file_path = test.empty() ? test : JSON_DIR / version / test;
            capiocl::printer::print(capiocl::printer::CLI_LEVEL_WARNING,
                                    "Testing on file " + test_file_path.string());

            EXPECT_THROW(capiocl::parser::Parser::parse(test_file_path),
                         capiocl::parser::ParserException);
        }
    }
}

TEST(EXCEPTION_SUITE_NAME, testWrongCommitRule) {
    bool exception_caught = false;
    try {
        capiocl::engine::Engine engine;
        engine.setCommitRule("x", "failMe");
    } catch (const std::invalid_argument &e) {
        exception_caught = true;
    } catch (...) {
        exception_caught = false;
    }

    EXPECT_TRUE(exception_caught);
}

TEST(EXCEPTION_SUITE_NAME, testWrongFireRule) {
    bool exception_caught = false;
    try {
        capiocl::engine::Engine engine;
        engine.setFireRule("x", "failMe");
    } catch (const std::invalid_argument &e) {
        exception_caught = true;
    } catch (...) {
        exception_caught = false;
    }

    EXPECT_TRUE(exception_caught);
}

#endif // CAPIO_CL_EXCEPTIONS_HPP