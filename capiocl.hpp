#ifndef CAPIO_CL_CAPIOCL_HPP
#define CAPIO_CL_CAPIOCL_HPP

#include <jsoncons/basic_json.hpp>
#include <string>

/// @brief Namespace containing all the CAPIO-CL related code
namespace capiocl {

/// @brief Default workflow name for CAPIO-CL
constexpr char CAPIO_CL_DEFAULT_WF_NAME[] = "CAPIO_CL";

/// @brief Namespace containing the CAPIO-CL Firing Rules
namespace fireRules {
/// @brief FnU Streaming Rule
constexpr char NO_UPDATE[] = "no_update";
/// @brief FoC Streaming Rule
constexpr char UPDATE[]    = "update";

/**
 * Sanitize fire rule from input
 * @param input
 * @return sanitized fire rule
 */
inline std::string sanitize(const std::string &input) {
    if (input == NO_UPDATE) {
        return NO_UPDATE;
    } else if (input == UPDATE) {
        return UPDATE;
    } else {
        throw std::invalid_argument("Input fire rule: " + input + " is not a valid CAPIO-CL rule");
    }
}
} // namespace fireRules

/// @brief Namespace containing the CAPIO-CL Commit Rules
namespace commitRules {
/// @brief CoC Streaming Rule
constexpr char ON_CLOSE[]       = "on_close";
/// @brief CoF Streaming Rule
constexpr char ON_FILE[]        = "on_file";
/// @brief CnF Streaming Rule
constexpr char ON_N_FILES[]     = "on_n_files";
/// @brief CoT Streaming Rule
constexpr char ON_TERMINATION[] = "on_termination";

/**
 * Sanitize commit rule from input
 * @param input
 * @return sanitized commit rule
 */
inline std::string sanitize(const std::string &input) {
    if (input == ON_CLOSE) {
        return ON_CLOSE;
    } else if (input == ON_FILE) {
        return ON_FILE;
    } else if (input == ON_N_FILES) {
        return ON_N_FILES;
    } else if (input == ON_TERMINATION) {
        return ON_TERMINATION;
    } else {
        throw std::invalid_argument("Input commit rule: " + input +
                                    " is not a valid CAPIO-CL rule");
    }
}
} // namespace commitRules

/// @brief Available versions of CAPIO-CL language
struct CAPIO_CL_VERSION final {
    /// @brief Release 1.0 of CAPIO-CL
    static constexpr char V1[]   = "1.0";
    /// @brief Release 1.1 of CAPIO-CL
    static constexpr char V1_1[] = "1.1";
};

namespace serializer {
class Serializer;
class SerializerException;
} // namespace serializer

namespace monitor {
class Monitor;
class MonitorException;
} // namespace monitor

namespace engine {
class Engine;
}

namespace configuration {
class CapioClConfiguration;
class CapioClConfigurationException;
struct defaults;
} // namespace configuration

namespace webapi {
class CapioClWebApiServer;
}

} // namespace capiocl

#endif // CAPIO_CL_CAPIOCL_HPP