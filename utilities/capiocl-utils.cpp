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

// ----------------- Helper: capture std::cout -----------------
class CaptureBuf : public std::stringbuf {
  public:
    std::string get_and_clear() {
        std::string out = str();
        str(""); // clear
        return out;
    }
};

// ----------------- Helper: render ANSI text in ncurses -----------------
void render_ansi_to_window(WINDOW *win, const std::string &text, int start_y, int start_x) {
    static bool colors_init = false;
    if (!colors_init) {
        start_color();
        use_default_colors();
        init_pair(30, COLOR_BLACK, -1);
        init_pair(31, COLOR_RED, -1);
        init_pair(32, COLOR_GREEN, -1);
        init_pair(33, COLOR_YELLOW, -1);
        init_pair(34, COLOR_BLUE, -1);
        init_pair(35, COLOR_MAGENTA, -1);
        init_pair(36, COLOR_CYAN, -1);
        init_pair(37, COLOR_WHITE, -1);
        colors_init = true;
    }

    int y       = start_y;
    int x       = start_x;
    int attr_on = 0;

    auto set_color = [&](int code) {
        if (code == 0) {
            if (attr_on != 0) {
                wattroff(win, COLOR_PAIR(attr_on));
                attr_on = 0;
            }
            return;
        }
        if (code >= 30 && code <= 37) {
            if (attr_on != 0) {
                wattroff(win, COLOR_PAIR(attr_on));
            }
            wattron(win, COLOR_PAIR(code));
            attr_on = code;
        }
    };

    for (size_t i = 0; i < text.size();) {
        if (i + 2 < text.size() && text[i] == '\x1b' && text[i + 1] == '[') {
            size_t mpos = text.find('m', i + 2);
            if (mpos == std::string::npos) {
                break;
            }
            std::string seq = text.substr(i + 2, mpos - (i + 2));
            std::stringstream ss(seq);
            std::string tok;
            while (getline(ss, tok, ';')) {
                if (!tok.empty()) {
                    set_color(std::atoi(tok.c_str()));
                }
            }
            i = mpos + 1;
            continue;
        }

        if (text[i] == '\n') {
            y++;
            x = start_x;
            int maxy, maxx;
            getmaxyx(win, maxy, maxx);
            if (y >= maxy - 1) {
                wscrl(win, 1);
                y = maxy - 2;
            }
            ++i;
            continue;
        }

        // Printable char
        mvwaddch(win, y, x, text[i]);
        ++x;
        int maxy, maxx;
        getmaxyx(win, maxy, maxx);
        if (x >= maxx - 1) {
            x = start_x;
            y++;
            if (y >= maxy - 1) {
                wscrl(win, 1);
                y = maxy - 2;
            }
        }
        ++i;
    }
}

// ----------------- Main function -----------------
void capio_cl_builder() {
    initscr();
    cbreak();
    noecho();
    curs_set(1);

    int height, width;
    getmaxyx(stdscr, height, width);

    // Stack windows vertically
    int cli_height = 6;
    int top_height = height - cli_height;

    WINDOW *top = newwin(top_height, width, 0, 0);
    WINDOW *cli = newwin(cli_height, width, top_height, 0);

    scrollok(top, TRUE);
    scrollok(cli, TRUE);

    box(top, 0, 0);
    box(cli, 0, 0);
    mvwprintw(top, 0, 2, " Engine Output ");
    mvwprintw(cli, 0, 2, " CAPIO-CL Builder ");
    wrefresh(top);
    wrefresh(cli);

    capiocl::Engine engine;
    bool terminate = false;
    char input[256];

    while (!terminate) {
        // Prompt in bottom window
        werase(cli);
        box(cli, 0, 0);
        mvwprintw(cli, 0, 2, " CAPIO-CL Builder ");
        mvwprintw(cli, 2, 2, "command> ");
        wmove(cli, 2, 11);
        echo();
        wrefresh(cli);

        wgetnstr(cli, input, sizeof(input) - 1);
        noecho();

        std::string line(input);
        auto args = split(line, ' ');
        if (args.empty()) {
            continue;
        }
        const std::string &command = args[0];

        if (command == "exit") {
            terminate = true;
        } else if (command == "help") {
            mvwprintw(cli, 4, 2,
                      "Commands:\n"
                      "\thelp - Show this menu\n"
                      "\texit - Quit CAPIO-CL builder\n"
                      "\tsave <file> - Save workflow\n"
                      "\tadd <file>  - Add new file\n");
        } else if (command == "save") {
            if (args.size() < 2) {
                mvwprintw(cli, 4, 2, "Error: missing filename\n");
            } else {
                capiocl::Serializer::dump(engine, "TODO:WORKFLOW_NAME", args[1]);
            }
        } else if (command == "add") {
            if (args.size() < 2) {
                mvwprintw(cli, 4, 2, "Error: missing filename\n");
            } else {
                engine.newFile(args[1]);
            }
        } else {
            mvwprintw(cli, 4, 2, "Unknown command: %s\n", command.c_str());
        }

        // Capture engine output and render with ANSI colors
        CaptureBuf cap;
        std::streambuf *old_buf = std::cout.rdbuf(&cap);
        engine.print();
        std::cout.flush();
        std::cout.rdbuf(old_buf);

        std::string out = cap.get_and_clear();

        werase(top);
        box(top, 0, 0);
        mvwprintw(top, 0, 2, " Engine Output ");
        render_ansi_to_window(top, out, 1, 2);
        wrefresh(top);

        // Clear CLI after each command
        werase(cli);
        box(cli, 0, 0);
        mvwprintw(cli, 0, 2, " CAPIO-CL Builder ");
        wrefresh(cli);
    }

    endwin();
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