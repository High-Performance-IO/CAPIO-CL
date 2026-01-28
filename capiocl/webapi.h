#ifndef CAPIO_CL_WEBAPI_H
#define CAPIO_CL_WEBAPI_H
#include <thread>

#include "capiocl.hpp"

class capiocl::webapi::CapioClWebApiServer {

    std::thread _webApiThread;
    int _port;

    char _secretKey[256];

  public:
    CapioClWebApiServer(engine::Engine *engine, const std::string &web_server_address,
                        int web_server_port);
    ~CapioClWebApiServer();
};

#endif // CAPIO_CL_WEBAPI_H
