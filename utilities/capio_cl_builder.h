#ifndef CAPIO_CL_UI_H
#define CAPIO_CL_UI_H

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <ncurses.h>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

constexpr char HELP_MESSAGE_COMMANDS[] =
    "\n"
    "=====================================================================\n"
    "                        CAPIO-CL BUILDER HELP                        \n"
    "=====================================================================\n"
    "\n"
    "General Commands:\n"
    "  help                           Show this help menu\n"
    "  exit                           Quit CAPIO-CL builder\n"
    "  print                          Print current configuration\n"
    "  save <filename>                Save configuration to <file>\n"
    "\n"
    "Add Commands:\n"
    "  add file <file_name>           Add a new file to the workflow\n"
    "  add producer <name> <file>     Add a producer process for <file>\n"
    "  add consumer <name> <file>     Add a consumer process for <file>\n"
    "  add dependency <dep> <file>    Add dependency <dep> for <file>\n"
    "\n"
    "Set Commands:\n"
    "  set name <workflow_name>       Set the workflow name\n"
    "  set memory <file>              Mark <file> as stored in memory\n"
    "  set fs <file>                  Mark <file> as stored on filesystem\n"
    "  set permanent <file>           Mark <file> as permanent\n"
    "  set exclude <file>             Exclude <file> from workflow output\n"
    "  set directory <file>           Mark <file> as directory\n"
    "  set file <file>                Mark <file> as regular file\n"
    "  set committed <file> <rule>    Set commit rule for file\n"
    "  set fire <file> <rule>         Set fire rule for file\n"
    "  set close <file> <count>       Set number of close for commit_on_close:N\n"
    "  set nfiles <file> <count>      Set number of files inside directory\n"
    "\n"
    "Unset Commands:\n"
    "  unset permanent <file>         Remove 'permanent' flag from <file>\n"
    "  unset exclude <file>           Remove 'exclude' flag from <file>\n"
    "\n"
    "Delete Commands:\n"
    "  delete <file>                  Delete a file from the configuration\n"
    "\n"
    "=====================================================================\n";

class CaptureBuf : public std::stringbuf {
  public:
    std::string get_and_clear() {
        std::string out = str();
        str(""); // clear
        return out;
    }
};

inline void render_ansi_to_window(WINDOW *pad, const std::string &text, int start_y, int start_x) {
    // Regex to match ANSI escape sequences (like color codes)
    static const std::regex ansi_regex("\x1B\\[[0-9;?]*[ -/]*[@-~]");
    static const std::regex remove_capio_cl_pre(R"(\[CAPIO-CL [^\]]*\])");
    static const std::regex remove_first_capiocl_row(".*Composition of expected CAPIO FS:.*\\n?");

    std::string clean = std::regex_replace(text, ansi_regex, "");
    clean             = std::regex_replace(clean, remove_capio_cl_pre, "");
    clean             = std::regex_replace(clean, remove_first_capiocl_row, "");

    int y = start_y;
    int x = start_x;

    for (char c : clean) {
        if (c == '\n') {
            y++;
            x = start_x;
        } else {
            mvwaddch(pad, y, x, c);
            x++;
        }

        // Let pad grow naturally; no wrapping limit
        if (x >= 1000) { // arbitrary max pad width safety cap
            x = start_x;
            y++;
        }
    }
}


inline void print_top_text(WINDOW *top, const std::string &title, const std::string &text) {
    werase(top);
    box(top, 0, 0);
    mvwprintw(top, 0, 2, "%s", title.c_str());

    render_ansi_to_window(top, text, 1, 2);

    wrefresh(top);
    doupdate();
}

inline void print_server_state(WINDOW *top, capiocl::Engine engine) {
    CaptureBuf cap;
    std::streambuf *old_buf = std::cout.rdbuf(&cap);
    engine.print();
    std::cout.flush();
    std::cout.rdbuf(old_buf);
    print_top_text(top, " Engine Output ", cap.get_and_clear());
}

inline std::tuple<bool, std::string> handle_add_command(std::vector<std::string> &args,
                                                        capiocl::Engine &engine) {
    const std::string &add_type = args[0];
    std::string error_message;
    bool error_occurred = false;

    if (add_type == "file") {
        engine.newFile(args[1]);
    } else if (add_type == "producer") {
        std::string &producer_name     = args[1];
        const std::string &target_file = args[2];
        engine.addProducer(target_file, producer_name);
    } else if (add_type == "consumer") {
        std::string &consumer_name     = args[1];
        const std::string &target_file = args[2];
        engine.addConsumer(target_file, consumer_name);
    } else if (add_type == "dependency") {
        std::filesystem::path dependency(args[1]);
        const std::string &target_file = args[2];
        engine.addFileDependency(target_file, dependency);
    } else {
        error_message  = "Unknown subcommand for add: " + args[2];
        error_occurred = true;
    }

    return {error_occurred, error_message};
}

inline std::tuple<bool, std::string> handle_save_command(std::vector<std::string> &args,
                                                         capiocl::Engine &engine) {
    std::string error_message;
    bool error_occurred = false;

    if (args.size() < 2) {
        error_message  = "Missing filename";
        error_occurred = true;
    } else {
        capiocl::Serializer::dump(engine, "TODO:WORKFLOW_NAME", args[1]);
    }
    return {error_occurred, error_message};
}

inline std::tuple<bool, std::string>
handle_print_command([[maybe_unused]] std::vector<std::string> &args,
                     [[maybe_unused]] capiocl::Engine &engine) {
    return {false, ""};
}

inline std::tuple<bool, std::string>
handle_help_command([[maybe_unused]] std::vector<std::string> &args,
                    [[maybe_unused]] capiocl::Engine &engine) {
    return {true, ""};
}

inline std::tuple<bool, std::string> handle_set_command(std::vector<std::string> &args,
                                                        capiocl::Engine &engine) {

    std::string error_message;
    bool error_occurred       = false;
    const std::string &target = args[0];
    if (target == "workflow_name") {
        error_message = " WARNING ",
        "Setting workflow name is not yet supported by CAPIO-CL: " + args[1];
        error_occurred = true;
    } else if (target == "memory") {
        const std::string &target_file = args[1];
        engine.setStoreFileInMemory(target_file);
    } else if (target == "fs") {
        const std::string &target_file = args[1];
        engine.setStoreFileInFileSystem(target_file);
    } else if (target == "permanent") {
        const std::string &target_file = args[1];
        engine.setPermanent(target_file, true);
    } else if (target == "exclude") {
        const std::string &target_file = args[1];
        engine.setExclude(target_file, true);
    } else if (target == "directory") {
        const std::string &target_file = args[1];
        engine.setDirectory(target_file);
    } else if (target == "file") {
        const std::string &target_file = args[1];
        engine.setFile(target_file);
    } else if (target == "commit") {
        const std::string &target_file = args[1];
        const std::string &rule        = args[2];
        engine.setCommitRule(target_file, rule);
    } else if (target == "fire") {
        const std::string &target_file = args[1];
        const std::string &rule        = args[2];
        engine.setFireRule(target_file, rule);
    } else if (target == "close") {
        const std::string &target_file = args[1];
        const auto &count              = std::stoi(args[2]);
        engine.setCommitedCloseNumber(target_file, count);
    } else if (target == "nfiles") {
        const std::string &target_file = args[1];
        const auto &count              = std::stoi(args[2]);
        engine.setDirectoryFileCount(target_file, count);
    } else {
        error_message  = "Unknown subcommand for set: " + args[1];
        error_occurred = true;
    }
    return {error_occurred, error_message};
}

inline std::tuple<bool, std::string> handle_unset_command(std::vector<std::string> &args,
                                                          capiocl::Engine &engine) {

    std::string error_message;
    bool error_occurred       = false;
    const std::string &target = args[0];
    if (target == "permanent") {
        const std::string &target_file = args[1];
        engine.setPermanent(target_file, false);
    } else if (target == "exclude") {
        const std::string &target_file = args[1];
        engine.setExclude(target_file, false);
    } else {
        error_message  = "Unknown subcommand for set: " + args[1];
        error_occurred = true;
    }
    return {error_occurred, error_message};
}

inline std::tuple<bool, std::string> handle_delete_command(std::vector<std::string> &args,
                                                           capiocl::Engine &engine) {
    const std::string &file = args[0];
    engine.remove(file);

    return {false, ""};
}

inline void capio_cl_builder() {
    initscr();
    cbreak();
    noecho();
    curs_set(1);

    int height, width;
    getmaxyx(stdscr, height, width);

    // Stack vertically
    int cli_height = 10;
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

    print_server_state(top, engine);

    std::unordered_map<std::string, std::tuple<bool, std::string> (*)(std::vector<std::string> &,
                                                                      capiocl::Engine &)>
        command_handlers;

    command_handlers["add"]    = &handle_add_command;
    command_handlers["save"]   = &handle_save_command;
    command_handlers["print"]  = &handle_print_command;
    command_handlers["help"]   = &handle_help_command;
    command_handlers["set"]    = &handle_set_command;
    command_handlers["unset"]  = &handle_unset_command;
    command_handlers["delete"] = &handle_delete_command;

    while (!terminate) {

        std::string error_message;
        bool error_occurred = false;

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

        std::string command = args.front();
        args.erase(args.begin());

        if (command == "exit") {
            terminate = true;
            continue;
        }

        if (command_handlers.find(command) != command_handlers.end()) {
            std::tie(error_occurred, error_message) = command_handlers[command](args, engine);
        } else {
            error_message  = "Unknown command: " + command;
            error_occurred = true;
        }

        if (error_occurred) {
            error_message += HELP_MESSAGE_COMMANDS;
            print_top_text(top, " Error ", error_message);
        } else {
            print_server_state(top, engine);
        }

        werase(cli);
        box(cli, 0, 0);
        mvwprintw(cli, 0, 2, " CAPIO-CL Builder ");
        wrefresh(cli);
    }

    endwin();
}

#endif // CAPIO_CL_UI_H
