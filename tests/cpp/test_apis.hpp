#ifndef CAPIO_CL_TEST_APIS_HPP
#define CAPIO_CL_TEST_APIS_HPP

#define WEBSERVER_SUITE_NAME TestWebServerAPIS

#include <arpa/inet.h>
#include <iostream>
#include <jsoncons/json.hpp>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "capiocl.hpp"
#include "capiocl/engine.h"

#include <string>

inline bool sendMulticast(const std::string &message, const std::string &multicast_ip, int port) {
    const int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("socket");
        return false;
    }

    const u_char ttl = 3;
    if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0) {
        perror("setsockopt (TTL)");
        close(fd);
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = inet_addr(multicast_ip.c_str());
    addr.sin_port        = htons(port);

    ssize_t nbytes = sendto(fd, message.c_str(), message.size(), 0,
                            reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));

    if (nbytes < 0) {
        perror("sendto");
        close(fd);
        return false;
    }

    close(fd);
    return true;
}

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
    EXPECT_FALSE(engine.contains("file.txt"));

    EXPECT_TRUE(
        sendMulticast(R"({"path" : "file.txt", "workflow_name" : "notMyWorkflow"})",
                      capiocl::configuration::defaults::DEFAULT_API_MULTICAST_IP.v,
                      stoi(capiocl::configuration::defaults::DEFAULT_API_MULTICAST_PORT.v)));

    EXPECT_TRUE(
        sendMulticast("notAValidJson", capiocl::configuration::defaults::DEFAULT_API_MULTICAST_IP.v,
                      stoi(capiocl::configuration::defaults::DEFAULT_API_MULTICAST_PORT.v)));

    EXPECT_TRUE(
        sendMulticast(R"({"workflow_name" : "notMyWorkflow", "CapioClEntry":{}})",
                      capiocl::configuration::defaults::DEFAULT_API_MULTICAST_IP.v,
                      stoi(capiocl::configuration::defaults::DEFAULT_API_MULTICAST_PORT.v)));

    EXPECT_TRUE(
        sendMulticast(R"({"path" : "file.txt", "CapioClEntry":{}})",
                      capiocl::configuration::defaults::DEFAULT_API_MULTICAST_IP.v,
                      stoi(capiocl::configuration::defaults::DEFAULT_API_MULTICAST_PORT.v)));

    EXPECT_TRUE(
        sendMulticast(R"({"path" : "file.txt", "workflow_name" : "notMyWorkflow"})",
                      capiocl::configuration::defaults::DEFAULT_API_MULTICAST_IP.v,
                      stoi(capiocl::configuration::defaults::DEFAULT_API_MULTICAST_PORT.v)));

    capiocl::engine::CapioCLEntry entry;
    entry.commit_rule           = "on_file";
    entry.commit_on_close_count = 10;
    entry.fire_rule             = "no_update";

    EXPECT_TRUE(sendMulticast(
        R"({ "path" : "file.txt","workflow_name" : "notMyWorkflow", "CapioClEntry":)" +
            entry.toJson() + "}",
        capiocl::configuration::defaults::DEFAULT_API_MULTICAST_IP.v,
        stoi(capiocl::configuration::defaults::DEFAULT_API_MULTICAST_PORT.v)));

    std::string request = R"({ "path" : "file.txt","workflow_name" : ")";
    request += capiocl::CAPIO_CL_DEFAULT_WF_NAME;
    request += R"(", "CapioClEntry":)" + entry.toJson() + "}";

    EXPECT_TRUE(
        sendMulticast(request, capiocl::configuration::defaults::DEFAULT_API_MULTICAST_IP.v,
                      stoi(capiocl::configuration::defaults::DEFAULT_API_MULTICAST_PORT.v)));

    // timeout to allow server to process the request
    while (!engine.contains("file.txt")) {
        sleep(1);
    }

    EXPECT_TRUE(engine.contains("file.txt"));
    EXPECT_EQ(engine.getCommitRule("file.txt"), entry.commit_rule);
    EXPECT_EQ(engine.getCommitCloseCount("file.txt"), entry.commit_on_close_count);
    EXPECT_EQ(engine.getFireRule("file.txt"), entry.fire_rule);
}

#endif // CAPIO_CL_TEST_APIS_HPP
