#include <filesystem>
#include <fstream>
#include <jsoncons/json.hpp>

#include "capiocl.hpp"
#include "capiocl/engine.h"
#include "capiocl/printer.h"
#include "capiocl/serializer.h"

void capiocl::serializer::Serializer::dump(const engine::Engine &engine,
                                           const std::filesystem::path &filename,
                                           const bool compress, const std::string &version) {
    if (version == CAPIO_CL_VERSION::V1) {
        printer::print(printer::CLI_LEVEL_INFO, "Serializing engine with V1 specification");
        available_serializers::serialize_v1(engine, filename, compress);
    } else if (version == CAPIO_CL_VERSION::V1_1) {
        printer::print(printer::CLI_LEVEL_INFO, "Serializing engine with V1.1 specification");
        available_serializers::serialize_v1_1(engine, filename, compress);
    } else {
        const auto message = "No serializer available for CAPIO-CL version: " + version;
        throw SerializerException(message);
    }
}

capiocl::serializer::SerializerException::SerializerException(const std::string &msg)
    : message(msg) {
    printer::print(printer::CLI_LEVEL_ERROR, msg);
}