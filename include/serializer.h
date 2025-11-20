#ifndef CAPIO_CL_SERIALIZER_H
#define CAPIO_CL_SERIALIZER_H

#include "capiocl.hpp"

/// @brief Namespace containing the CAPIO-CL Serializer component
namespace capiocl::serializer {

/**
 * @brief Custom exception thrown when serializing an instance of Engine
 */
class SerializerException final : public std::exception {
    std::string message;

  public:
    /**
     * @brief Construct a new CAPIO-CL Exception
     * @param msg Error Message that raised this exception
     */
    explicit SerializerException(const std::string &msg);
    /**
     * Get the description of the error causing the exception
     * @return
     */
    [[nodiscard]] const char *what() const noexcept override { return message.c_str(); }
};

/// @brief Dump the current loaded CAPIO-CL configuration from class Engine to a CAPIO-CL
/// configuration file.
class Serializer final {

    /// @brief Available serializers for CAPIO-CL
    struct available_serializers {
        /**
         * @brief Dump the current configuration loaded into an instance of  Engine to a CAPIO-CL
         * VERSION 1 configuration file.
         *
         * @param engine instance of Engine to dump
         * @param filename path of output file
         * @throws SerializerException
         */
        static void serialize_v1(const engine::Engine &engine,
                                 const std::filesystem::path &filename);

        /**
         * @brief Dump the current configuration loaded into an instance of  Engine to a CAPIO-CL
         * VERSION 1.1 configuration file.
         *
         * @param engine instance of Engine to dump
         * @param filename path of output file
         * @throws SerializerException
         */
        static void serialize_v1_1(const engine::Engine &engine,
                                   const std::filesystem::path &filename);
    };

  public:
    /**
     * @brief Dump the current configuration loaded into an instance of Engine to a CAPIO-CL
     * configuration file.
     *
     * @param engine instance of Engine to dump
     * @param filename path of output file
     * @param version Version of CAPIO-CL used to generate configuration files.
     */
    static void dump(const engine::Engine &engine, const std::filesystem::path &filename,
                     const std::string &version = CAPIO_CL_VERSION::V1);
};
} // namespace capiocl::serializer
#endif // CAPIO_CL_SERIALIZER_H