#ifndef CAPIO_CL_EXCEPTIONS_HPP
#define CAPIO_CL_EXCEPTIONS_HPP

#define EXCEPTION_SUITE_NAME TestThrowExceptions
#include "capiocl/serializer.h"

TEST(EXCEPTION_SUITE_NAME, testWhatMEthods) {
    try {
        capiocl::parser::Parser::parse("");
    } catch (const capiocl::parser::ParserException &e) {
        EXPECT_TRUE(demangled_name(e) == "capiocl::parser::ParserException");
        EXPECT_GT(strlen(e.what()), 0);
    }

    try {
        const auto engine = capiocl::engine::Engine();
        capiocl::serializer::Serializer::dump(engine, "");
    } catch (const capiocl::serializer::SerializerException &e) {
        EXPECT_TRUE(demangled_name(e) == "capiocl::serializer::SerializerException");
        EXPECT_GT(strlen(e.what()), 0);
    }
}

TEST(EXCEPTION_SUITE_NAME, testFailedDump) {
    for (const auto &version : CAPIO_CL_AVAIL_VERSIONS) {
        const std::filesystem::path source = "/tmp/capio_cl_jsons/V" + version + "/test24.json";
        auto engine                        = capiocl::parser::Parser::parse(source, "/tmp");

        EXPECT_THROW(capiocl::serializer::Serializer::dump(*engine, "/"),
                     capiocl::serializer::SerializerException);
    }
}

TEST(EXCEPTION_SUITE_NAME, testFailedserializeVersion) {
    for (const auto &version : CAPIO_CL_AVAIL_VERSIONS) {
        const std::filesystem::path source = "/tmp/capio_cl_jsons/V" + version + "/test24.json";
        auto engine                        = capiocl::parser::Parser::parse(source, "/tmp");

        EXPECT_THROW(capiocl::serializer::Serializer::dump(*engine, "test.json", "1234.5678"),
                     capiocl::serializer::SerializerException);
    }
}

TEST(EXCEPTION_SUITE_NAME, testParserException) {
    std::filesystem::path JSON_DIR = "/tmp/capio_cl_jsons/";
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
    for (const auto &version : CAPIO_CL_AVAIL_VERSIONS) {
        for (const auto &test : test_filenames) {
            const auto test_file_path = test.empty() ? test : JSON_DIR / ("V" + version) / test;
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