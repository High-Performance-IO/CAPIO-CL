#include "httplib.h"
#include "jsoncons/json.hpp"

#include "capiocl/engine.h"
#include "capiocl/printer.h"
#include "capiocl/webapi.h"

template <typename Res> void ok_response(Res &res) {
    res.status = 200;
    res.set_content(R"({"status" : "OK"})", "application/json");
}

template <typename Res> void error_response(Res &res, const std::exception &e) {
    res.status = 400;
    res.set_content(std::string(R"({"status" : "error", "what" : ")") +
                        "Invalid request BODY data: " + e.what() + "\"}",
                    "application/json");
}

template <typename Res> void json_response(Res &res, const jsoncons::json &body) {
    res.status = 200;
    res.set_content(body.as_string(), "application/json");
}

template <typename Req, typename Res, typename Fn>
void process_post_request(const Req &req, Res &res, Fn &&handler) {
    try {
        jsoncons::json request_body = jsoncons::json::parse(req.body.empty() ? "{}" : req.body);

        handler(request_body);
        ok_response(res);
    } catch (const std::exception &e) {
        error_response(res, e);
    }
}

template <typename Req, typename Res, typename Fn>
void process_get_request(const Req &req, Res &res, Fn &&handler) {
    try {
        jsoncons::json request_body = jsoncons::json::parse(req.body.empty() ? "{}" : req.body);

        jsoncons::json reply;
        handler(request_body, reply);
        json_response(res, reply);
    } catch (const std::exception &e) {
        error_response(res, e);
    }
}

/// @brief Main WebServer thread function
void server(const std::string &address, const int port, capiocl::engine::Engine *engine) {

    capiocl::printer::print(capiocl::printer::CLI_LEVEL_INFO,
                            "Starting API server @ " + address + ":" + std::to_string(port));

    httplib::Server _server;

    _server.Post("/producer", [&](const httplib::Request &req, httplib::Response &res) {
        process_post_request(req, res, [&](jsoncons::json &request_body) {
            const auto path = request_body["path"].as<std::string>();
            auto producer   = request_body["producer"].as<std::string>();
            engine->addProducer(path, producer);
        });
    });

    _server.Get("/producer", [&](const httplib::Request &req, httplib::Response &res) {
        process_get_request(req, res, [&](jsoncons::json &request_body, jsoncons::json &reply) {
            const auto path    = request_body["path"].as<std::string>();
            reply["producers"] = engine->getProducers(path);
        });
    });

    _server.Post("/consumer", [&](const httplib::Request &req, httplib::Response &res) {
        process_post_request(req, res, [&](jsoncons::json &request_body) {
            const auto path = request_body["path"].as<std::string>();
            auto consumer   = request_body["consumer"].as<std::string>();
            engine->addConsumer(path, consumer);
        });
    });

    _server.Get("/consumer", [&](const httplib::Request &req, httplib::Response &res) {
        process_get_request(req, res, [&](jsoncons::json &request_body, jsoncons::json &reply) {
            const auto path    = request_body["path"].as<std::string>();
            reply["consumers"] = engine->getConsumers(path);
        });
    });

    _server.Post("/dependency", [&](const httplib::Request &req, httplib::Response &res) {
        process_post_request(req, res, [&](jsoncons::json &request_body) {
            const auto path = request_body["path"].as<std::string>();
            auto dependency = std::filesystem::path(request_body["dependency"].as<std::string>());
            engine->addFileDependency(path, dependency);
        });
    });

    _server.Get("/dependency", [&](const httplib::Request &req, httplib::Response &res) {
        process_get_request(req, res, [&](jsoncons::json &request_body, jsoncons::json &reply) {
            const auto path = request_body["path"].as<std::string>();
            std::vector<std::string> deps;
            for (const auto &file : engine->getCommitOnFileDependencies(path)) {
                deps.emplace_back(file);
            }
            reply["dependencies"] = deps;
        });
    });

    _server.Post("/commit", [&](const httplib::Request &req, httplib::Response &res) {
        process_post_request(req, res, [&](jsoncons::json &request_body) {
            const auto path  = request_body["path"].as<std::string>();
            auto commit_rule = request_body["commit"].as<std::string>();
            engine->setCommitRule(path, commit_rule);
        });
    });

    _server.Get("/commit", [&](const httplib::Request &req, httplib::Response &res) {
        process_get_request(req, res, [&](jsoncons::json &request_body, jsoncons::json &reply) {
            const auto path = request_body["path"].as<std::string>();
            reply["commit"] = engine->getCommitRule(path);
        });
    });

    _server.Post("/commit/file-count", [&](const httplib::Request &req, httplib::Response &res) {
        process_post_request(req, res, [&](jsoncons::json &request_body) {
            const auto path = request_body["path"].as<std::string>();
            auto count      = request_body["count"].as<int>();
            engine->setDirectoryFileCount(path, count);
        });
    });

    _server.Get("/commit/file-count", [&](const httplib::Request &req, httplib::Response &res) {
        process_get_request(req, res, [&](jsoncons::json &request_body, jsoncons::json &reply) {
            const auto path = request_body["path"].as<std::string>();
            reply["count"]  = engine->getDirectoryFileCount(path);
        });
    });

    _server.Post("/commit/close-count", [&](const httplib::Request &req, httplib::Response &res) {
        process_post_request(req, res, [&](jsoncons::json &request_body) {
            const auto path = request_body["path"].as<std::string>();
            auto count      = request_body["count"].as<int>();
            engine->setCommitedCloseNumber(path, count);
        });
    });

    _server.Get("/commit/close-count", [&](const httplib::Request &req, httplib::Response &res) {
        process_get_request(req, res, [&](jsoncons::json &request_body, jsoncons::json &reply) {
            const auto path = request_body["path"].as<std::string>();
            reply["count"]  = engine->getCommitCloseCount(path);
        });
    });

    _server.Post("/fire", [&](const httplib::Request &req, httplib::Response &res) {
        process_post_request(req, res, [&](jsoncons::json &request_body) {
            const auto path = request_body["path"].as<std::string>();
            auto fire_rule  = request_body["fire"].as<std::string>();
            engine->setFireRule(path, fire_rule);
        });
    });

    _server.Get("/fire", [&](const httplib::Request &req, httplib::Response &res) {
        process_get_request(req, res, [&](jsoncons::json &request_body, jsoncons::json &reply) {
            const auto path = request_body["path"].as<std::string>();
            reply["fire"]   = engine->getFireRule(path);
        });
    });

    _server.Post("/permanent", [&](const httplib::Request &req, httplib::Response &res) {
        process_post_request(req, res, [&](jsoncons::json &request_body) {
            const auto path      = request_body["path"].as<std::string>();
            const auto permanent = request_body["permanent"].as<bool>();
            engine->setPermanent(path, permanent);
        });
    });

    _server.Get("/permanent", [&](const httplib::Request &req, httplib::Response &res) {
        process_get_request(req, res, [&](jsoncons::json &request_body, jsoncons::json &reply) {
            const auto path    = request_body["path"].as<std::string>();
            reply["permanent"] = engine->isPermanent(path);
        });
    });

    _server.Post("/exclude", [&](const httplib::Request &req, httplib::Response &res) {
        process_post_request(req, res, [&](jsoncons::json &request_body) {
            const auto path     = request_body["path"].as<std::string>();
            const auto excluded = request_body["exclude"].as<bool>();
            engine->setExclude(path, excluded);
        });
    });

    _server.Get("/exclude", [&](const httplib::Request &req, httplib::Response &res) {
        process_get_request(req, res, [&](jsoncons::json &request_body, jsoncons::json &reply) {
            const auto path  = request_body["path"].as<std::string>();
            reply["exclude"] = engine->isExcluded(path);
        });
    });

    _server.Post("/directory", [&](const httplib::Request &req, httplib::Response &res) {
        process_post_request(req, res, [&](jsoncons::json &request_body) {
            const auto path = request_body["path"].as<std::string>();
            if (request_body["directory"].as<bool>()) {
                engine->setDirectory(path);
            } else {
                engine->setFile(path);
            }
        });
    });

    _server.Get("/directory", [&](const httplib::Request &req, httplib::Response &res) {
        process_get_request(req, res, [&](jsoncons::json &request_body, jsoncons::json &reply) {
            const auto path    = request_body["path"].as<std::string>();
            reply["directory"] = engine->isDirectory(path);
        });
    });

    _server.Post("/workflow", [&](const httplib::Request &req, httplib::Response &res) {
        process_post_request(req, res, [&](jsoncons::json &request_body) {
            const auto workflow_name = request_body["name"].as<std::string>();
            engine->setWorkflowName(workflow_name);
        });
    });

    _server.Get("/workflow", [&](const httplib::Request &req, httplib::Response &res) {
        process_get_request(
            req, res, [&]([[maybe_unused]] jsoncons::json &request_body, jsoncons::json &reply) {
                reply["name"] = engine->getWorkflowName();
            });
    });

    _server.Get("/terminate", [&]([[maybe_unused]] const httplib::Request &req,
                                  [[maybe_unused]] httplib::Response &res) {
        process_get_request(req, res,
                            [&]([[maybe_unused]] jsoncons::json &request_body,
                                [[maybe_unused]] jsoncons::json &reply) {
                                capiocl::printer::print(capiocl::printer::CLI_LEVEL_INFO,
                                                        "API server stopped");
                                _server.stop();
                            });
    });

    _server.listen(address, port);
}

capiocl::webapi::CapioClWebApiServer::CapioClWebApiServer(engine::Engine *engine,
                                                          const std::string &web_server_address,
                                                          const int web_server_port)
    : _port(web_server_port) {
    _webApiThread = std::thread(server, web_server_address, web_server_port, engine);
}

capiocl::webapi::CapioClWebApiServer::~CapioClWebApiServer() {

    httplib::Client client("http://127.0.0.1:" + std::to_string(_port));
    client.Get("/terminate");
    if (_webApiThread.joinable()) {
        _webApiThread.join();
    } else {
        return;
    }
}
