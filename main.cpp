#include <iostream>
#include <fmt/core.h>
#include <__algorithm/ranges_contains.h>

#include "database.hpp"
#include "Logger.hpp"

void repl(Database &db);

int Logger::trace_level = 0;

auto main(const int argc, char *argv[]) -> int {
    int trace_level = 0;

    for (int i = 1; i < argc; ++i) {
        if (std::string arg = argv[i]; arg.rfind("--trace-level=", 0) == 0) {
            try {
                std::string level_str = arg.substr(14);
                trace_level = std::stoi(level_str);
                if (trace_level < 0) {
                    throw std::invalid_argument("Trace level cannot be negative.");
                }
            } catch (const std::exception &e) {
                std::cerr << "Invalid trace level value. It should be either 0 or 1. Instead it is: " << arg <<
                        std::endl;
                return 1;
            }
        }
    }
    Logger::set_trace_level(trace_level);

    const auto db_config = DatabaseConfig(1);
    auto db = Database(db_config);
    repl(db);
    return 0;
}

void display_help();

std::string trim_leading_spaces(const std::string &str) {
    const auto it = std::ranges::find_if(str, [](const unsigned char ch) {
        return !std::isspace(ch);
    });
    return std::string(it, str.end());
}


void repl(Database &db) {
    namespace rg = std::ranges;

    fmt::println("EdgyDB v1.0.0");
    fmt::println("Type 'help' for list of options.");

    auto const exit_commands = std::vector<std::string>{"exit", "quit"};

    while (true) {
        fmt::print("> ");
        std::string command;
        std::getline(std::cin, command);
        command = trim_leading_spaces(command);
        if (rg::contains(exit_commands, command)) break;
        if (command == "help") {
            display_help();
            continue;
        }
        db.execute_query(Query::from_string(command));
    }
}

void display_help() {
    fmt::println("To exit REPL type 'exit' or 'quit'");
    fmt::println("Available DDL commands:");
    fmt::println("CREATE [TYPE/EDGE/NODE]");
    fmt::println("Available DML commands:");
    fmt::println("To be documented...");
    fmt::println("Available DQL commands:");
    fmt::println("To be documented...");
}
