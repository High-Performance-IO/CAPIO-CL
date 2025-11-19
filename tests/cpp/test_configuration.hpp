#ifndef CAPIO_CL_TEST_CONFIGURATION_HPP
#define CAPIO_CL_TEST_CONFIGURATION_HPP

#define CONFIGURATION_SUITE_NAME TestTOMLConfiguration

TEST(CONFIGURATION_SUITE_NAME, TestLoadConfiguration) {
    capiocl::engine::Engine engine;
    engine.loadConfiguration("/tmp/capio_cl_tomls/sample1.toml");
    EXPECT_TRUE(true);
}

#endif // CAPIO_CL_TEST_CONFIGURATION_HPP
