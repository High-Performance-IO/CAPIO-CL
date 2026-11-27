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

#include "capiocl.hpp"
#include "capiocl/engine.h"

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
    "  set commit <file> <rule>       Set commit rule for file\n"
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

inline std::tuple<bool, std::string> handle_add_command(std::vector<std::string> &args,
                                                        capiocl::engine::Engine &engine) {
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
                                                         capiocl::engine::Engine &engine) {
    std::string error_message;
    bool error_occurred = false;

    if (args.empty()) {
        error_message  = "Missing filename";
        error_occurred = true;
    } else {
        capiocl::serializer::Serializer::dump(engine, args[1]);
    }
    return {error_occurred, error_message};
}

inline std::tuple<bool, std::string>
handle_print_command([[maybe_unused]] std::vector<std::string> &args,
                     [[maybe_unused]] capiocl::engine::Engine &engine) {
    return {false, ""};
}

inline std::tuple<bool, std::string>
handle_help_command([[maybe_unused]] std::vector<std::string> &args,
                    [[maybe_unused]] capiocl::engine::Engine &engine) {
    return {true, ""};
}

inline std::tuple<bool, std::string> handle_set_command(std::vector<std::string> &args,
                                                        capiocl::engine::Engine &engine) {

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
                                                          capiocl::engine::Engine &engine) {

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
                                                           capiocl::engine::Engine &engine) {
    const std::string &file = args[0];
    engine.remove(file);

    return {false, ""};
}

inline void draw_engine_table(WINDOW *top, const capiocl::engine::Engine &engine) {
    werase(top);
    box(top, 0, 0);
    mvwprintw(top, 0, 2, " CAPIO-CL Engine State ");

    int win_height, win_width;
    getmaxyx(top, win_height, win_width);

    std::vector<std::string> headers = {"File",  "Producers", "Consumers", "Commit", "Fire",
                                        "Store", "Excluded",  "Permanent", "N_files"};

    std::vector min_widths   = {12, 18, 18, 18, 10, 7, 9, 10, 8};
    std::vector dynamic_cols = {0};

    int total_fixed = 0;
    for (size_t i = 0; i < headers.size(); ++i) {
        if (std::find(dynamic_cols.begin(), dynamic_cols.end(), i) == dynamic_cols.end()) {
            total_fixed += min_widths[i] + 1;
        }
    }

    //  dynamic columns
    int available    = std::max(0, win_width - 4 - total_fixed); // borders & padding
    int base_dynamic = 0;
    for (auto idx : dynamic_cols) {
        base_dynamic += min_widths[idx];
    }

    int extra_per_col = available > base_dynamic ? available - base_dynamic : 0;
    int extra_each    = extra_per_col / (int) dynamic_cols.size();

    std::vector<int> col_widths = min_widths;
    for (auto idx : dynamic_cols) {
        col_widths[idx] += extra_each;
    }

    struct Row {
        std::string file, stored, producers, consumers, commit, fire;
        bool excluded, permanent;
        long nfiles;
    };

    std::vector<Row> rows;

    for (const auto &file : engine.getPaths()) {
        Row row;
        row.file   = file;
        row.stored = engine.isStoredInMemory(file) ? "MEM" : "FS";

        std::ostringstream prod, cons;
        for (auto &p : engine.getProducers(file)) {
            prod << p << " ";
        }
        for (auto &c : engine.getConsumers(file)) {
            cons << c << " ";
        }
        row.producers = prod.str();
        row.consumers = cons.str();

        row.commit    = engine.getCommitRule(file);
        row.fire      = engine.getFireRule(file);
        row.excluded  = engine.isExcluded(file);
        row.permanent = engine.isPermanent(file);
        row.nfiles    = engine.getCommitCloseCount(file);

        rows.push_back(row);
    }

    int y = 1;
    int x = 2;

    // ---- HEADER ----
    wattron(top, COLOR_PAIR(1) | A_BOLD);
    for (size_t i = 0; i < headers.size(); ++i) {
        mvwprintw(top, y, x, "%-*.*s", col_widths[i], col_widths[i], headers[i].c_str());
        x += col_widths[i] + 1;
    }
    wattroff(top, COLOR_PAIR(1) | A_BOLD);

    y++;

    // ---- ROWS ----
    int row_idx = 0;
    for (auto &r : rows) {
        if (y >= win_height - 1) {
            break;
        }

        // Alternate background colors
        int pair_id = (row_idx % 2 == 0) ? 2 : 3;
        wattron(top, COLOR_PAIR(pair_id));

        x                               = 2;
        std::vector<std::string> values = {r.file,
                                           r.producers,
                                           r.consumers,
                                           r.commit,
                                           r.fire,
                                           r.stored,
                                           r.excluded ? "yes" : "no",
                                           r.permanent ? "yes" : "no",
                                           std::to_string(r.nfiles)};

        for (size_t i = 0; i < values.size(); ++i) {
            std::string v = values[i];
            if ((int) v.size() > col_widths[i]) {
                v = v.substr(0, col_widths[i] - 1) + "…";
            }
            mvwprintw(top, y, x, "%-*.*s", col_widths[i], col_widths[i], v.c_str());
            x += col_widths[i] + 1;
        }

        wattroff(top, COLOR_PAIR(pair_id));
        y++;
        row_idx++;
    }

    wattroff(top, A_DIM);
    wrefresh(top);
}

inline void capio_cl_builder() {
    initscr();
    start_color();
    use_default_colors();
    cbreak();
    noecho();
    curs_set(1);

    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
    init_pair(3, COLOR_RED, COLOR_BLACK);

    int height, width;
    getmaxyx(stdscr, height, width);

    // Stack vertically
    int cli_height = 4;
    int top_height = height - cli_height;

    WINDOW *top = newwin(top_height, width, 0, 0);
    WINDOW *cli = newwin(cli_height, width, top_height, 0);

    scrollok(top, TRUE);
    scrollok(cli, TRUE);

    box(top, 0, 0);
    box(cli, 0, 0);
    mvwprintw(top, 0, 2, " Engine Output ");
    mvwprintw(cli, 0, 2, " Command interface ");
    wrefresh(top);
    wrefresh(cli);

    capiocl::engine::Engine engine;
    bool terminate = false;
    char input[256];

    draw_engine_table(top, engine);

    std::unordered_map<std::string, std::tuple<bool, std::string> (*)(std::vector<std::string> &,
                                                                      capiocl::engine::Engine &)>
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
        mvwprintw(cli, 0, 2, " Command ");
        mvwprintw(cli, 2, 2, "> ");
        wmove(cli, 2, 4);
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
            draw_engine_table(top, engine);
        }

        werase(cli);
        box(cli, 0, 0);
        mvwprintw(cli, 0, 2, " CAPIO-CL Builder ");
        wrefresh(cli);
    }

    endwin();
}

#endif // CAPIO_CL_UI_H
