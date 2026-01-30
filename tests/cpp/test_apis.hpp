#ifndef CAPIO_CL_TEST_APIS_HPP
#define CAPIO_CL_TEST_APIS_HPP

#define WEBSERVER_SUITE_NAME TestWebServerAPIS

#include <curl/curl.h>

#include <curl/curl.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

enum class HttpMethod { GET, POST, DELETE };

static size_t write_cb(const char *ptr, const size_t size, size_t nmemb, void *userdata) {
    auto *response = static_cast<std::string *>(userdata);
    response->append(ptr, size * nmemb);
    return size * nmemb;
}

static std::unordered_map<std::string, std::string> from_json(const std::string &json) {
    std::unordered_map<std::string, std::string> result;
    size_t pos = 0;

    while (true) {
        auto k1 = json.find('"', pos);
        if (k1 == std::string::npos) {
            break;
        }
        const auto k2 = json.find('"', k1 + 1);

        const auto v1 = json.find('"', k2 + 1);
        const auto v2 = json.find('"', v1 + 1);

        std::string key       = json.substr(k1 + 1, k2 - k1 - 1);
        const std::string val = json.substr(v1 + 1, v2 - v1 - 1);

        result[key] = val;
        pos         = v2 + 1;
    }
    return result;
}

inline std::unordered_map<std::string, std::string>
perform_request(const std::string &url, const std::string &request_params_json_encode,
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
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
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
    if (http_code < 200 || http_code >= 300) {
        throw std::runtime_error("HTTP error " + std::to_string(http_code));
    }

    return from_json(response);
}

TEST(WEBSERVER_SUITE_NAME, testGetAndSetWorkflowName) {

    const auto engine = capiocl::engine::Engine();

    sleep(1);

    auto response = perform_request("http://localhost:5520/workflow", "{}", HttpMethod::GET);
    if (response["name"] != capiocl::CAPIO_CL_DEFAULT_WF_NAME) {
        for (const auto &[key, val] : response) {
            std::cout << key << " : " << val << std::endl;
        }
    }


    EXPECT_FALSE(response.empty());
    EXPECT_TRUE(response["name"] == capiocl::CAPIO_CL_DEFAULT_WF_NAME);


    perform_request("http://localhost:5520/workflow", R"({"name": "test_workflow_0"})",
                    HttpMethod::POST);
    response = perform_request("http://localhost:5520/workflow", "{}", HttpMethod::GET);
    EXPECT_FALSE(response.empty());
    EXPECT_TRUE(response["name"] == "test_workflow_0");
}

#endif // CAPIO_CL_TEST_APIS_HPP
