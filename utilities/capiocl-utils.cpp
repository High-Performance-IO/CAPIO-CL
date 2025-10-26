#include <args.hxx>
#include <iostream>
#include <ncurses.h>

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

std::vector<std::string> split(const std::string &str, const char delimiter) {
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string token;

    while (std::getline(ss, token, delimiter)) {
        result.push_back(token);
    }

    return result;
}

void capio_cl_builder() {
    bool terminate = false;

    capiocl::Engine engine;

    while (!terminate) {
        std::cout << "command> ";
        std::string input;
        getline(std::cin, input);

        auto args           = split(input, ' ');
        const auto &command = args[0];

        if (command == "exit") {
            terminate = true;
        } else if (command == "help") {
            std::cout << "Command availables:" << std::endl
                      << "\thelp: Show this menu" << std::endl
                      << "\texit: Exit from CAPIO-CL builder" << std::endl;
        } else if (command == "save") {
            if (args.size() < 2) {
                std::cerr << "Please enter output filename. Args size: " << args.size()
                          << std::endl;
                continue;
            }
            capiocl::Serializer::dump(engine, "TODO:WORKFLOW_NAME", args[1]);
        } else if (command == "add") {
            if (args.size() < 2) {
                std::cerr << "Please enter input filename. Args size: " << args.size();
                continue;
            }
            engine.newFile(args[1]);
        }
    }
    std::cout << "Bye!" << std::endl;
}

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
        } catch (...) {
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
        capio_cl_builder();
    }
}