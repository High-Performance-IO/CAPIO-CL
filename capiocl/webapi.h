#ifndef CAPIO_CL_WEBAPI_H
#define CAPIO_CL_WEBAPI_H
#include <thread>

#include "capiocl.hpp"

/// @brief Class that exposes a REST Web Server to interact with the current configuration
class capiocl::webapi::CapioClWebApiServer {

    /// @brief asynchronous running webserver thread
    std::thread _webApiThread;

    /// @brief port on which the current server runs
    int _port;

  public:
    /// @brief default constructor.
    CapioClWebApiServer(engine::Engine *engine, const std::string &web_server_address,
                        int web_server_port);

    /// @brief Default Destructor
    ~CapioClWebApiServer();
};

#endif // CAPIO_CL_WEBAPI_H
