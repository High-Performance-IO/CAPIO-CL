#include "capiocl.hpp"
#include <filesystem>
#include <fstream>
#include <jsoncons/json.hpp>

void capiocl::Serializer::dump(const Engine &engine, const std::string &workflow_name,
                               const std::filesystem::path &filename, const std::string &version) {
    if (version == CAPIO_CL_VERSION::V1) {
        print_message(CLI_LEVEL_INFO, "Serializing engine with V1 specification");
        available_serializers::serialize_v1(engine, workflow_name, filename);
    } else {
        const auto message = "No serializer available for CAPIO-CL version: " + version;
        throw SerializerException(message);
    }
}