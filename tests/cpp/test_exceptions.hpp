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
    std::filesystem::path JSON_DIR = "/tmp/capio_cl_jsons";
    capiocl::printer::print(capiocl::printer::CLI_LEVEL_INFO,
                            "Loading jsons from " + JSON_DIR.string());
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
        JSON_DIR / "V1_test22.json",
        JSON_DIR / "V1_test23.json",
        JSON_DIR / "V1_test25.json",
    };

    for (const auto &test : test_filenames) {
        exception_catched = false;
        try {
            capiocl::printer::print(capiocl::printer::CLI_LEVEL_WARNING,
                                    "Testing on file " + test.string());
            capiocl::parser::Parser::parse(test);
        } catch (std::exception &e) {
            exception_catched = true;
            auto demangled    = demangled_name(e);
            capiocl::printer::print(capiocl::printer::CLI_LEVEL_INFO,
                                    "Caught exception of type =" + demangled);
            EXPECT_TRUE(demangled == "capiocl::parser::ParserException");
            EXPECT_GT(std::string(e.what()).size(), 0);
        }
        EXPECT_TRUE(exception_catched);
        capiocl::printer::print(capiocl::printer::CLI_LEVEL_INFO, "Test failed successfully\n\n");
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