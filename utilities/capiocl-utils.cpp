#include <args.hxx>
#include <atomic>
#include <chrono>
#include <iostream>
#include <ncurses.h>
#include <string>
#include <thread>

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

class Printer {
  public:
    void run(WINDOW *win, std::atomic<bool> &running) {
        int counter = 0;
        while (running) {
            // Print a line on the right side
            wprintw(win, "Right side output: %d\n", counter++);
            wrefresh(win);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
};

void capio_cl_builder() {
    initscr();   // Start ncurses
    cbreak();    // Disable line buffering
    noecho();    // Don’t echo typed characters
    curs_set(1); // Show the cursor

    int height, width;
    getmaxyx(stdscr, height, width);

    int left_width  = width / 2;
    int right_width = width - left_width;

    // Create two windows
    WINDOW *left  = newwin(height, left_width, 0, 0);
    WINDOW *right = newwin(height, right_width, 0, left_width);

    // Draw borders
    box(left, 0, 0);
    box(right, 0, 0);
    mvwprintw(left, 0, 2, " Console ");
    mvwprintw(right, 0, 2, " Output ");
    wrefresh(left);
    wrefresh(right);

    std::atomic<bool> running(true);
    Printer printer;
    std::thread printer_thread([&] { printer.run(right, running); });

    // Interactive input on the left
    char input[256];
    int row = 1;
    while (true) {
        mvwprintw(left, row, 2, "> ");
        wrefresh(left);

        wgetnstr(left, input, sizeof(input) - 1);

        std::string cmd(input);
        if (cmd == "quit" || cmd == "exit") {
            break;
        }

        row++;
        if (row >= height - 1) {
            werase(left);
            box(left, 0, 0);
            mvwprintw(left, 0, 2, " Console ");
            row = 1;
        }

        mvwprintw(left, row, 2, "You typed: %s", input);
        wrefresh(left);
        row++;
    }

    running = false;
    printer_thread.join();

    endwin(); // Restore terminal
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