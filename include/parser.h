#ifndef CAPIO_CL_PARSER_H
#define CAPIO_CL_PARSER_H
#include <jsoncons/basic_json.hpp>
#include <jsoncons_ext/jsonschema/jsonschema.hpp>

/// @brief Namespace containing the CAPIO-CL parsers components
namespace capiocl::parser {

/**
 * @brief Custom exception thrown when parsing a CAPIO-CL configuration file by Parser
 */
class ParserException final : public std::exception {
    std::string message;

  public:
    /**
     * @brief Construct a new CAPIO-CL Exception
     * @param msg Error Message that raised this exception
     */
    explicit ParserException(const std::string &msg);

    /**
     * Get the description of the error causing the exception
     * @return
     */
    [[nodiscard]] const char *what() const noexcept override { return message.c_str(); }
};

/// @brief Contains the code to parse a JSON based CAPIO-CL configuration file
class Parser final {

    /// @brief Available parsers for CAPIO-CL
    struct available_parsers {
        /**
         * Parser for the V1 Specification of the CAPIO-CL language
         * @param source Path of CAPIO-CL configuration file
         * @param resolve_prefix Prefix to prepend to path if found to be relative
         * @param store_only_in_memory Flag to set to returned instance of Engine if required to
         * store all files in memory
         * @return Parsed Engine.
         */
        static engine::Engine *parse_v1(const std::filesystem::path &source,
                                        const std::filesystem::path &resolve_prefix,
                                        bool store_only_in_memory);

        /**
         * Parser for the V1.1 Specification of the CAPIO-CL language
         * @param source Path of CAPIO-CL configuration file
         * @param resolve_prefix Prefix to prepend to path if found to be relative
         * @param store_only_in_memory Flag to set to returned instance of Engine if required to
         * store all files in memory
         * @return Parsed Engine.
         */
        static engine::Engine *parse_v1_1(const std::filesystem::path &source,
                                          const std::filesystem::path &resolve_prefix,
                                          bool store_only_in_memory);
    };

    /**
     * Load a json Schema into memory from a byte encoded array castable to a const char[]
     * @param data Array of byte encoded json Schema
     * @return The generated
     */
    static jsoncons::jsonschema::json_schema<jsoncons::json> loadSchema(const char *data);

  protected:
    /**
     * Resolve (if relative) a path to an absolute one using the provided prefix
     * @param path Path to resolve
     * @param prefix Prefix
     * @return Absolute path constructed from path
     */
    static std::filesystem::path resolve(std::filesystem::path path,
                                         const std::filesystem::path &prefix);

  public:
    /**
     * Validate a CAPIO-CL configuration file according to the JSON schema of the language
     * @param doc The loaded CAPIO-CL configuration file
     * @param str_schema Raw JSON schema to use
     */
    static void validate_json(const jsoncons::json &doc, const char *str_schema);

    /**
     * @brief Perform the parsing of the capio_server configuration file
     *
     * @param source Input CAPIO-CL Json configuration File
     * @param resolve_prefix If paths are found to be relative, they are appended to this path
     * @param store_only_in_memory Set to true to set all files to be stored in memory
     * @return Engine instance with the information provided by  the config file
     * @throw ParserException
     */
    static engine::Engine *parse(const std::filesystem::path &source,
                                 const std::filesystem::path &resolve_prefix = "",
                                 bool store_only_in_memory                   = false);
};
} // namespace capiocl::parser

#endif // CAPIO_CL_PARSER_H