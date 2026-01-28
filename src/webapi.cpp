#include "httplib.h"
#include "jsoncons/json.hpp"

#include "capiocl/engine.h"
#include "capiocl/printer.h"
#include "capiocl/webapi.h"

#define OK_RESPONSE(res)                                                                           \
    res.status = 200;                                                                              \
    res.set_content("OK", "text/plain");

#define ERROR_RESPONSE(res, e)                                                                     \
    res.status = 400;                                                                              \
    res.set_content("{\"error\" : \"" + std::string("Invalid request BODY data: ") + e.what() +    \
                        "\"}",                                                                     \
                    "text/plain");

#define JSON_RESPONSE(res, json_body)                                                              \
    res.status = 200;                                                                              \
    res.set_content(json_body.as_string(), "application/json");

#define PROCESS_POST_REQUEST(req, res, code)                                                       \
    try {                                                                                          \
        jsoncons::json request_body = jsoncons::json::parse(req.body.empty() ? "{}" : req.body);   \
        code;                                                                                      \
        OK_RESPONSE(res);                                                                          \
    } catch (const std::exception &e) {                                                            \
        ERROR_RESPONSE(res, e);                                                                    \
    }

#define PROCESS_GET_REQUEST(req, res, code)                                                        \
    try {                                                                                          \
        jsoncons::json request_body = jsoncons::json::parse(req.body.empty() ? "{}" : req.body);   \
        jsoncons::json reply;                                                                      \
        code;                                                                                      \
        JSON_RESPONSE(res, reply);                                                                 \
    } catch (const std::exception &e) {                                                            \
        ERROR_RESPONSE(res, e);                                                                    \
    }

void server(const std::string &address, int port, capiocl::engine::Engine *engine);

capiocl::webapi::CapioClWebApiServer::CapioClWebApiServer(engine::Engine *engine,
                                                          const std::string &web_server_address,
                                                          const int web_server_port)
    : _webApiThread(std::thread(server, web_server_address, web_server_port, engine)) {

    printer::print(printer::CLI_LEVEL_INFO, "API server started on " + web_server_address + ":" +
                                                std::to_string(web_server_port));
}

capiocl::webapi::CapioClWebApiServer::~CapioClWebApiServer() {
    pthread_cancel(_webApiThread.native_handle());
    _webApiThread.join();
    printer::print(printer::CLI_LEVEL_INFO, "API server stopped");
}

void server(const std::string &address, const int port, capiocl::engine::Engine *engine) {

    httplib::Server _server;

    _server.Post("/new", [&](const httplib::Request &req, httplib::Response &res) {
        PROCESS_POST_REQUEST(req, res, {
            const auto path = request_body["path"].as<std::string>();
            engine->newFile(path);
        })
    });

    _server.Post("/producer", [&](const httplib::Request &req, httplib::Response &res) {
        PROCESS_POST_REQUEST(req, res, {
            const auto path = request_body["path"].as<std::string>();
            auto producer   = request_body["producer"].as<std::string>();
            engine->addProducer(path, producer);
        });
    });

    _server.Get("/producer", [&](const httplib::Request &req, httplib::Response &res) {
        PROCESS_GET_REQUEST(req, res, {
            const auto path    = request_body["path"].as<std::string>();
            reply["producers"] = engine->getProducers(path);
        });
    });

    _server.Post("/consumer", [&](const httplib::Request &req, httplib::Response &res) {
        PROCESS_POST_REQUEST(req, res, {
            const auto path = request_body["path"].as<std::string>();
            auto consumer   = request_body["consumer"].as<std::string>();
            engine->addConsumer(path, consumer);
        });
    });

    _server.Get("/consumer", [&](const httplib::Request &req, httplib::Response &res) {
        PROCESS_GET_REQUEST(req, res, {
            const auto path    = request_body["path"].as<std::string>();
            reply["consumers"] = engine->getConsumers(path);
        });
    });

    _server.Post("/dependency", [&](const httplib::Request &req, httplib::Response &res) {
        PROCESS_POST_REQUEST(req, res, {
            const auto path = request_body["path"].as<std::string>();
            auto dependency = std::filesystem::path(request_body["dependency"].as<std::string>());
            engine->addFileDependency(path, dependency);
        });
    });

    _server.Get("/dependency", [&](const httplib::Request &req, httplib::Response &res) {
        PROCESS_GET_REQUEST(req, res, {
            const auto path = request_body["path"].as<std::string>();
            std::vector<std::string> deps;
            for (const auto file : engine->getCommitOnFileDependencies(path)) {
                deps.emplace_back(file.string());
            }
            reply["dependencies"] = deps;
        });
    });

    _server.Post("/commit", [&](const httplib::Request &req, httplib::Response &res) {
        PROCESS_POST_REQUEST(req, res, {
            const auto path  = request_body["path"].as<std::string>();
            auto commit_rule = request_body["commit"].as<std::string>();
            engine->setCommitRule(path, commit_rule);
        });
    });

    _server.Get("/commit", [&](const httplib::Request &req, httplib::Response &res) {
        PROCESS_GET_REQUEST(req, res, {
            const auto path = request_body["path"].as<std::string>();
            reply["commit"] = engine->getCommitRule(path);
        });
    });

    _server.Post("/commit/file-count", [&](const httplib::Request &req, httplib::Response &res) {
        PROCESS_POST_REQUEST(req, res, {
            const auto path = request_body["path"].as<std::string>();
            auto count      = request_body["count"].as<int>();
            engine->setDirectoryFileCount(path, count);
        });
    });

    _server.Get("/commit/file-count", [&](const httplib::Request &req, httplib::Response &res) {
        PROCESS_GET_REQUEST(req, res, {
            const auto path = request_body["path"].as<std::string>();
            reply["count"]  = engine->getDirectoryFileCount(path);
        });
    });

    _server.Post("/commit/close-count", [&](const httplib::Request &req, httplib::Response &res) {
        PROCESS_POST_REQUEST(req, res, {
            const auto path = request_body["path"].as<std::string>();
            auto count      = request_body["count"].as<int>();
            engine->setCommitedCloseNumber(path, count);
        });
    });

    _server.Get("/commit/close-count", [&](const httplib::Request &req, httplib::Response &res) {
        PROCESS_GET_REQUEST(req, res, {
            const auto path = request_body["path"].as<std::string>();
            reply["count"]  = engine->getCommitCloseCount(path);
        });
    });

    _server.Post("/fire", [&](const httplib::Request &req, httplib::Response &res) {
        PROCESS_POST_REQUEST(req, res, {
            const auto path = request_body["path"].as<std::string>();
            auto fire_rule  = request_body["fire"].as<std::string>();
            engine->setFireRule(path, fire_rule);
        });
    });

    _server.Get("/fire", [&](const httplib::Request &req, httplib::Response &res) {
        PROCESS_GET_REQUEST(req, res, {
            const auto path = request_body["path"].as<std::string>();
            reply["fire"]   = engine->getFireRule(path);
        });
    });

    _server.Post("/permanent", [&](const httplib::Request &req, httplib::Response &res) {
        PROCESS_POST_REQUEST(req, res, {
            const auto path      = request_body["path"].as<std::string>();
            const auto permanent = request_body["permanent"].as<bool>();
            engine->setPermanent(path, permanent);
        });
    });

    _server.Get("/permanent", [&](const httplib::Request &req, httplib::Response &res) {
        PROCESS_GET_REQUEST(req, res, {
            const auto path    = request_body["path"].as<std::string>();
            reply["permanent"] = engine->isPermanent(path);
        });
    });

    _server.Post("/exclude", [&](const httplib::Request &req, httplib::Response &res) {
        PROCESS_POST_REQUEST(req, res, {
            const auto path     = request_body["path"].as<std::string>();
            const auto excluded = request_body["excluded"].as<bool>();
            engine->setExclude(path, excluded);
        });
    });

    _server.Get("/exclude", [&](const httplib::Request &req, httplib::Response &res) {
        PROCESS_GET_REQUEST(req, res, {
            const auto path  = request_body["path"].as<std::string>();
            reply["exclude"] = engine->isExcluded(path);
        });
    });

    _server.Post("/directory", [&](const httplib::Request &req, httplib::Response &res) {
        PROCESS_POST_REQUEST(req, res, {
            const auto path = request_body["path"].as<std::string>();
            if (request_body["directory"].as<bool>()) {
                engine->setDirectory(path);
            } else {
                engine->setFile(path);
            }
        });
    });

    _server.Get("/directory", [&](const httplib::Request &req, httplib::Response &res) {
        PROCESS_GET_REQUEST(req, res, {
            const auto path    = request_body["path"].as<std::string>();
            reply["directory"] = engine->isDirectory(path);
        });
    });

    _server.Post("/workflow", [&](const httplib::Request &req, httplib::Response &res) {
        PROCESS_POST_REQUEST(req, res, {
            const auto workflow_name = request_body["name"].as<std::string>();
            engine->setWorkflowName(workflow_name);
        });
    });

    _server.Get("/workflow", [&](const httplib::Request &req, httplib::Response &res) {
        PROCESS_GET_REQUEST(req, res, { reply["name"] = engine->getWorkflowName(); });
    });

    _server.Delete("/", [&](const httplib::Request &req, httplib::Response &res) {
        PROCESS_POST_REQUEST(req, res, {
            const auto path = request_body["path"].as<std::string>();
            engine->remove(path);
        });
    });

    _server.listen(address, port);
}