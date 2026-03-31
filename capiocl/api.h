#ifndef CAPIO_CL_WEBAPI_H
#define CAPIO_CL_WEBAPI_H
#include <atomic>
#include <thread>

#include "capiocl.hpp"
#include "configuration.h"

/// @brief Class that exposes a REST Web Server to interact with the current configuration
class capiocl::api::CapioClApiServer {

    /// @brief asynchronous running webserver thread
    std::thread _webApiThread;

    /// @brief port on which the current server runs
    const configuration::CapioClConfiguration &capiocl_configuration;

    /// @brief variable to tell the thread to terminate
    std::atomic<bool> _terminate = false;

  public:
    /// @brief default constructor.
    CapioClApiServer(engine::Engine *engine, configuration::CapioClConfiguration &config);

    /// @brief Default Destructor
    ~CapioClApiServer();
};

#endif // CAPIO_CL_WEBAPI_H
