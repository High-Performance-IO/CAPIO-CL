#ifndef CAPIO_CL_PRINTER_H
#define CAPIO_CL_PRINTER_H
#include <iostream>
#include <unistd.h>

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 1024
#endif

/// @brief Namespace containing the CAPIO-CL print utilities
namespace capiocl::printer {

/// @brief CLI print constant
constexpr char CLI_LEVEL_INFO[]    = "[\033[1;32mCAPIO-CL\033[0m";
/// @brief CLI print constant
constexpr char CLI_LEVEL_WARNING[] = "[\033[1;33mCAPIO-CL\033[0m";
/// @brief CLI print constant
constexpr char CLI_LEVEL_ERROR[]   = "[\033[1;31mCAPIO-CL\033[0m";
/// @brief CLI print constant
constexpr char CLI_LEVEL_JSON[]    = "[\033[1;34mCAPIO-CL\033[0m";

/**
 * Print a message to standard out. Used to log messages related to the CAPIO-CL Engine
 * @param message_type Type of message to print.
 * @param message_line
 */
inline void print(const std::string &message_type = "", const std::string &message_line = "") {
    static std::string *node_name = nullptr;
    if (node_name == nullptr) {
        node_name = new std::string(HOST_NAME_MAX, ' '); // LCOV_EXCL_LINE
        gethostname(node_name->data(), HOST_NAME_MAX);
    }
    if (message_type.empty()) {
        std::cout << std::endl;
    } else {
        std::cout << message_type << " " << node_name->c_str() << "] " << message_line << std::endl
                  << std::flush;
    }
}
} // namespace capiocl::printer

#endif // CAPIO_CL_PRINTER_H