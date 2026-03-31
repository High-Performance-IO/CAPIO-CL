#ifndef CAPIO_CL_TEST_APIS_HPP
#define CAPIO_CL_TEST_APIS_HPP

#define WEBSERVER_SUITE_NAME TestWebServerAPIS

#include "capiocl.hpp"
#include "capiocl/engine.h"

#include <string>

TEST(WEBSERVER_SUITE_NAME, TestSerializationDeserializationCapioCLRule) {
    capiocl::engine::CapioCLEntry entry;

    const std::string def_rule =
        "{\"commit_on_close_count\":0,\"commit_rule\":\"on_termination\",\"consumers\":[],"
        "\"directory_children_count\":0,\"enable_directory_count_update\":true,\"excluded\":false,"
        "\"file_dependencies\":[],\"fire_rule\":\"update\",\"is_file\":true,\"permanent\":false,"
        "\"producers\":[],\"store_in_memory\":false}";

    EXPECT_EQ(def_rule, entry.toJson());

    entry.commit_on_close_count         = 10;
    entry.commit_rule                   = "on_close";
    entry.consumers                     = {"aaaaa"};
    entry.producers                     = {"bbbbb"};
    entry.directory_children_count      = 12;
    entry.enable_directory_count_update = false;
    entry.excluded                      = true;
    entry.file_dependencies             = {"cccc"};
    entry.is_file                       = false;
    entry.permanent                     = true;
    entry.store_in_memory               = true;

    const std::string def_rule2 =
        "{\"commit_on_close_count\":10,\"commit_rule\":\"on_close\",\"consumers\":[\"aaaaa\"],"
        "\"directory_children_count\":12,\"enable_directory_count_update\":false,\"excluded\":true,"
        "\"file_dependencies\":[\"cccc\"],\"fire_rule\":\"update\",\"is_file\":false,\"permanent\":"
        "true,\"producers\":[\"bbbbb\"],\"store_in_memory\":true}";

    EXPECT_EQ(def_rule2, entry.toJson());

    auto new_rule = capiocl::engine::CapioCLEntry::fromJson(def_rule2);

    EXPECT_TRUE(new_rule == entry);

    entry.enable_directory_count_update = true;
    EXPECT_TRUE(entry != new_rule);
}

TEST(WEBSERVER_SUITE_NAME, TestWebServerAPIS) {
    capiocl::engine::Engine engine;
    engine.startApiServer();

}

#endif // CAPIO_CL_TEST_APIS_HPP
