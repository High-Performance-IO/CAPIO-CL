#ifndef CAPIO_CL_TEST_APIS_HPP
#define CAPIO_CL_TEST_APIS_HPP

#define WEBSERVER_SUITE_NAME TestWebServerAPIS

#include "jsoncons/json.hpp"
#include <curl/curl.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

enum class HttpMethod { GET, POST, DELETE };

static size_t curl_write_response_handler(const char *ptr, const size_t size, size_t nmemb,
                                          void *userdata) {
    auto *response = static_cast<std::string *>(userdata);
    response->append(ptr, size * nmemb);
    return size * nmemb;
}

inline jsoncons::json perform_request(const std::string &url,
                                      const std::string &request_params_json_encode,
                                      HttpMethod method = HttpMethod::GET) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("curl_easy_init failed");
    }

    std::string response;

    curl_slist *headers = nullptr;
    headers             = curl_slist_append(headers, "Content-Type: application/json");
    headers             = curl_slist_append(headers, "Accept: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_params_json_encode.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, request_params_json_encode.size());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_response_handler);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    switch (method) {
    case HttpMethod::GET:
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
        break;

    case HttpMethod::POST:
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
        break;

    case HttpMethod::DELETE:
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        break;
    }

    const CURLcode res = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        throw std::runtime_error(curl_easy_strerror(res));
    }

    std::cout << "DBG RES: " << response << std::endl;
    return jsoncons::json::parse(std::string(response));
}

TEST(WEBSERVER_SUITE_NAME, testGetAndSetWorkflowName) {

    // clean environment for wf name
    unsetenv("WORKFLOW_NAME");

    auto engine = capiocl::engine::Engine();
    engine.startApiServer();

    sleep(1);

    auto response = perform_request("http://localhost:5520/workflow", "{}", HttpMethod::GET);

    EXPECT_FALSE(response.empty());
    EXPECT_TRUE(response["name"] == capiocl::CAPIO_CL_DEFAULT_WF_NAME);

    perform_request("http://localhost:5520/workflow", R"({"name": "test_workflow_0"})",
                    HttpMethod::POST);
    response = perform_request("http://localhost:5520/workflow", "{}", HttpMethod::GET);
    EXPECT_FALSE(response.empty());
    EXPECT_TRUE(response["name"] == "test_workflow_0");
}

TEST(WEBSERVER_SUITE_NAME, consumer) {
    capiocl::engine::Engine engine;
    engine.startApiServer();
    sleep(1);

    auto result =
        perform_request("http://localhost:5520/consumer",
                        R"({"path" : "/tmp/test.txt", "consumer" : "sample2"})", HttpMethod::POST);
    EXPECT_TRUE(result["status"] == "OK");
    result = perform_request("http://localhost:5520/consumer", R"({"path" : "/tmp/test.txt"})",
                             HttpMethod::GET);
    EXPECT_TRUE(result["consumers"][0] == "sample2");
}

TEST(WEBSERVER_SUITE_NAME, producer) {
    capiocl::engine::Engine engine;
    engine.startApiServer();
    sleep(1);

    auto result =
        perform_request("http://localhost:5520/producer",
                        R"({"path" : "/tmp/test.txt", "producer" : "sample1"})", HttpMethod::POST);
    EXPECT_TRUE(result["status"] == "OK");
    result = perform_request("http://localhost:5520/producer", R"({"path" : "/tmp/test.txt"})",
                             HttpMethod::GET);
    EXPECT_TRUE(result["producers"][0] == "sample1");
}

TEST(WEBSERVER_SUITE_NAME, commit) {
    capiocl::engine::Engine engine;
    engine.startApiServer();
    sleep(1);

    auto result =
        perform_request("http://localhost:5520/commit",
                        R"({"path" : "/tmp/test.txt","commit" : "on_file"})", HttpMethod::POST);
    EXPECT_TRUE(result["status"] == "OK");
    result = perform_request("http://localhost:5520/commit", R"({"path" : "/tmp/test.txt"})",
                             HttpMethod::GET);
    EXPECT_TRUE(result["commit"] == "on_file");
}

TEST(WEBSERVER_SUITE_NAME, fire) {
    capiocl::engine::Engine engine;
    engine.startApiServer();
    sleep(1);

    auto result =
        perform_request("http://localhost:5520/fire",
                        R"({"path" : "/tmp/test.txt","fire" : "no_update"})", HttpMethod::POST);
    EXPECT_TRUE(result["status"] == "OK");
    result = perform_request("http://localhost:5520/fire", R"({"path" : "/tmp/test.txt"})",
                             HttpMethod::GET);
    EXPECT_TRUE(result["fire"] == "no_update");
}

TEST(WEBSERVER_SUITE_NAME, fileDependency) {
    capiocl::engine::Engine engine;
    engine.startApiServer();
    sleep(1);

    auto result = perform_request("http://localhost:5520/dependency",
                                  R"({"path" : "/tmp/test.txt", "dependency" : "myFile.dat"})",
                                  HttpMethod::POST);
    EXPECT_TRUE(result["status"] == "OK");
    result = perform_request("http://localhost:5520/dependency", R"({"path" : "/tmp/test.txt"})",
                             HttpMethod::GET);
    EXPECT_TRUE(result["dependencies"][0] == "myFile.dat");
}

TEST(WEBSERVER_SUITE_NAME, on_n_files) {
    capiocl::engine::Engine engine;
    engine.startApiServer();
    sleep(1);

    auto result = perform_request("http://localhost:5520/commit/file-count",
                                  R"({"path" : "/tmp/test.txt","count" : 7892})", HttpMethod::POST);
    EXPECT_TRUE(result["status"] == "OK");
    result = perform_request("http://localhost:5520/commit/file-count",
                             R"({"path" : "/tmp/test.txt"})", HttpMethod::GET);
    EXPECT_EQ(result["count"], 7892);
}

TEST(WEBSERVER_SUITE_NAME, close_count) {
    capiocl::engine::Engine engine;
    engine.startApiServer();
    sleep(1);

    auto result =
        perform_request("http://localhost:5520/commit/close-count",
                        R"({"path" : "/tmp/test.txt","count" : 12345})", HttpMethod::POST);
    EXPECT_TRUE(result["status"] == "OK");
    result = perform_request("http://localhost:5520/commit/close-count",
                             R"({"path" : "/tmp/test.txt"})", HttpMethod::GET);
    EXPECT_EQ(result["count"], 12345);
}

TEST(WEBSERVER_SUITE_NAME, test_error) {
    capiocl::engine::Engine engine;
    engine.startApiServer();
    sleep(1);

    auto result = perform_request("http://localhost:5520/commit", R"({})", HttpMethod::POST);
    EXPECT_TRUE(result["status"] == "error");
    EXPECT_GT(result["what"].as_string().size(), 0);
}

TEST(WEBSERVER_SUITE_NAME, boolean_flag) {

    capiocl::engine::Engine engine;
    engine.startApiServer();
    sleep(1);

    auto result =
        perform_request("http://localhost:5520/permanent",
                        R"({"path" : "/tmp/test.txt","permanent" : true})", HttpMethod::POST);
    EXPECT_TRUE(result["status"] == "OK");
    result = perform_request("http://localhost:5520/permanent", R"({"path" : "/tmp/test.txt"})",
                             HttpMethod::GET);
    EXPECT_TRUE(result["permanent"].as_bool());

    result = perform_request("http://localhost:5520/exclude",
                             R"({"path" : "/tmp/test.txt","exclude" : true})", HttpMethod::POST);
    EXPECT_TRUE(result["status"] == "OK");
    result = perform_request("http://localhost:5520/exclude", R"({"path" : "/tmp/test.txt"})",
                             HttpMethod::GET);
    EXPECT_TRUE(result["exclude"].as_bool());

    result = perform_request("http://localhost:5520/directory",
                             R"({"path" : "/tmp/test.txt","directory" : true})", HttpMethod::POST);
    EXPECT_TRUE(result["status"] == "OK");
    result = perform_request("http://localhost:5520/directory", R"({"path" : "/tmp/test.txt"})",
                             HttpMethod::GET);
    EXPECT_TRUE(result["directory"].as_bool());

    result = perform_request("http://localhost:5520/directory",
                             R"({"path" : "/tmp/test.txt","directory" : false})", HttpMethod::POST);
    EXPECT_TRUE(result["status"] == "OK");
    result = perform_request("http://localhost:5520/directory", R"({"path" : "/tmp/test.txt"})",
                             HttpMethod::GET);
    EXPECT_FALSE(result["directory"].as_bool());
}

#endif // CAPIO_CL_TEST_APIS_HPP
