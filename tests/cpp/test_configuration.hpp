#ifndef CAPIO_CL_TEST_CONFIGURATION_HPP
#define CAPIO_CL_TEST_CONFIGURATION_HPP

#define CONFIGURATION_SUITE_NAME TestTOMLConfiguration
#include "capiocl/configuration.h"

TEST(CONFIGURATION_SUITE_NAME, TestLoadConfiguration) {
    capiocl::engine::Engine engine;
    engine.loadConfiguration("/tmp/capio_cl_tomls/sample1.toml");
    EXPECT_TRUE(true);
}

TEST(CONFIGURATION_SUITE_NAME, TestLoadEmptyPath) {
    capiocl::engine::Engine engine;
    EXPECT_THROW(engine.loadConfiguration(""),
                 capiocl::configuration::CapioClConfigurationException);
}

TEST(CONFIGURATION_SUITE_NAME, TestGetParameter) {
    capiocl::configuration::CapioClConfiguration config;
    config.load("/tmp/capio_cl_tomls/sample1.toml");

    int int_value;
    config.getParameter("monitor.mcast.delay_ms", &int_value, -1);
    EXPECT_EQ(int_value, 300);
    config.getParameter("not.a.valid.key", &int_value, 42);
    EXPECT_EQ(int_value, 42);

    std::string string_value;
    config.getParameter("monitor.mcast.commit.ip", &string_value, "fallback");
    EXPECT_EQ(string_value, "224.224.224.3");
    config.getParameter("not.a.valid.key", &string_value, "fallback");
    EXPECT_EQ(string_value, "fallback");

    EXPECT_THROW(config.getParameter("workflow_name", &int_value, 0),
                 std::invalid_argument);
}

TEST(CONFIGURATION_SUITE_NAME, TestFailureParsingTOML) {
    capiocl::configuration::CapioClConfiguration config;
    EXPECT_THROW(config.load("/tmp/capio_cl_tomls/sample0.toml"),
                 capiocl::configuration::CapioClConfigurationException);
}

TEST(CONFIGURATION_SUITE_NAME, testNoBackendLoaded) {
    capiocl::engine::Engine engine(false);
    engine.loadConfiguration("/tmp/capio_cl_tomls/sample2.toml");

    EXPECT_FALSE(engine.isCommitted("test"));
}

TEST(CONFIGURATION_SUITE_NAME, testNoBackendLoadedWithExplicitNoLoadOption) {
    capiocl::configuration::CapioClConfiguration config;
    config.load("/tmp/capio_cl_tomls/sample3.toml");

    std::string value;
    config.getParameter("monitor.mcast.enabled", &value, "true");
    EXPECT_TRUE("false" == value);
    value = "";

    config.getParameter("monitor.filesystem.enabled", &value, "true");
    EXPECT_TRUE("false" == value);
}

#endif // CAPIO_CL_TEST_CONFIGURATION_HPP
