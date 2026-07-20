#include <filesystem>
#include <fstream>
#include <jsoncons/json.hpp>

#include "calf/StdOutLogger.h"
#include "calf/StlLogger.h"
#include "capiocl.hpp"
#include "capiocl/engine.h"
#include "capiocl/serializer.h"

void capiocl::serializer::Serializer::dump(const engine::Engine &engine,
                                           const std::filesystem::path &filename,
                                           const std::string &version) {
    START_LOG(calf_current_tid(), "call()");
    UPDATE_CALF_WORKFLOW_NAME(engine.getWorkflowName());
    if (version == CAPIO_CL_VERSION::V1) {
        CALF_PRINT_COLOR(CALF_CLI_LEVEL_INFO, "Serializing engine with V1 specification");
        available_serializers::serialize_v1(engine, filename);
    } else if (version == CAPIO_CL_VERSION::V1_1) {
        CALF_PRINT_COLOR(CALF_CLI_LEVEL_INFO, "Serializing engine with V1.1 specification");
        available_serializers::serialize_v1_1(engine, filename);
    } else {
        LOG("serializer unavailable version=%s workflow=%s output=%s", version.c_str(),
            engine.getWorkflowName().c_str(), filename.string().c_str());
        const auto message = "No serializer available for CAPIO-CL version: " + version;
        throw SerializerException(message);
    }
}

capiocl::serializer::SerializerException::SerializerException(const std::string &msg)
    : message(msg) {
    START_LOG(calf_current_tid(), "call()");
    UPDATE_CALF_WORKFLOW_NAME("");
    CALF_PRINT_COLOR(CALF_CLI_LEVEL_ERROR, "%s", msg.c_str());
}
