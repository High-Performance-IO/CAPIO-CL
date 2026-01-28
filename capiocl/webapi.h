#ifndef CAPIO_CL_WEBAPI_H
#define CAPIO_CL_WEBAPI_H
#include <thread>

#include "httplib.h"
#include "capiocl.hpp"

class capiocl::webapi::CapioClWebApiServer {

    std::thread _webApiThread;
    httplib::Server _webApiServer;

  public:
    CapioClWebApiServer(engine::Engine *engine, const std::string &web_server_address,
                        int web_server_port);
    ~CapioClWebApiServer();
};

#endif // CAPIO_CL_WEBAPI_H
