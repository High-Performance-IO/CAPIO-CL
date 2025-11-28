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

TEST(CONFIGURATION_SUITE_NAME, TestExceptions) {
    capiocl::configuration::CapioClConfiguration config;

    EXPECT_THROW(config.getParameter("not.a.valid.key", static_cast<int *>(nullptr)),
                 capiocl::configuration::CapioClConfigurationException);

    EXPECT_THROW(config.getParameter("not.a.valid.key", static_cast<std::string *>(nullptr)),
                 capiocl::configuration::CapioClConfigurationException);

    try {
        config.getParameter("not.a.valid.key", static_cast<std::string *>(nullptr));
    } catch (const capiocl::configuration::CapioClConfigurationException &err) {
        EXPECT_GT(strlen(err.what()), 0);
    }
}

TEST(CONFIGURATION_SUITE_NAME, TestFailureParsingTOML) {
    capiocl::configuration::CapioClConfiguration config;
    EXPECT_THROW(config.load("/tmp/capio_cl_tomls/sample0.toml"),
                 capiocl::configuration::CapioClConfigurationException);
}

#endif // CAPIO_CL_TEST_CONFIGURATION_HPP
