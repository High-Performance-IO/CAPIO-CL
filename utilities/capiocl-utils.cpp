#include <args.hxx>
#include <iostream>

#include "capiocl.hpp"

constexpr char capio_cl_header_help[] = R"(
  ______    ______   _______  ______   ______            ______   __
 /      \  /      \ |       \|      \ /      \          /      \ |  \
|  $$$$$$\|  $$$$$$\| $$$$$$$\\$$$$$$|  $$$$$$\        |  $$$$$$\| $$
| $$   \$$| $$__| $$| $$__/ $$ | $$  | $$  | $$ ______ | $$   \$$| $$
| $$      | $$    $$| $$    $$ | $$  | $$  | $$|      \| $$      | $$
| $$   __ | $$$$$$$$| $$$$$$$  | $$  | $$  | $$ \$$$$$$| $$   __ | $$
| $$__/  \| $$  | $$| $$      _| $$_ | $$__/ $$        | $$__/  \| $$_____
 \$$    $$| $$  | $$| $$     |   $$ \ \$$    $$         \$$    $$| $$     \
  \$$$$$$  \$$   \$$ \$$      \$$$$$$  \$$$$$$           \$$$$$$  \$$$$$$$$

                        CAPIO-CL Utilities
)";

int main(int argc, char **argv) {
    std::cout << capio_cl_header_help << std::endl;
    args::ArgumentParser parser(
        "CAPIO-CL Utilities",
        "Developed by Marco Edoardo Santimaria \n marcoedoardo.santimaria@unito.it");
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
    args::Group arguments(parser, "Arguments");
    args::ValueFlag<std::string> validate(
        arguments, "path", "Validate a CAPIO-CL configuration file", {'v', "validate"});
    args::Flag builder(arguments, "build", "Interactively build a CAPIO-CL configuration file",
                       {'b', "build"});

    try {
        parser.ParseCLI(argc, argv);
    } catch (const args::Completion &e) {
        std::cout << e.what();
        return 0;
    } catch (const args::Help &) {
        std::cout << parser;
        return 0;
    } catch (const args::ParseError &e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }

    if (validate) {
        const std::string path = args::get(validate);
        try {
            auto parsed = capiocl::Parser::parse(path);
        } catch (const capiocl::ParserException &e) {
            std::cerr << std::endl
                      << "\t+==================================================+\n"
                         "\t|\033[0;31m   Input File is NOT a VALID configuration file \033[0m  |\n"
                         "\t+==================================================+"
                      << std::endl;
            return 1;
        }

        std::cout << std::endl
                  << "\t+=============================================+\n"
                     "\t|\033[0;32m   Input File is a VALID configuration file \033[0m |\n"
                     "\t+=============================================+"
                  << std::endl;
        return 0;
    }

    if (builder) {
        std::cout << "Starting CAPIO-CL builder..." << std::endl;
    }
}