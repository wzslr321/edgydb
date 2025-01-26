#include <iostream>
#include <fmt/core.h>
#include <__algorithm/ranges_contains.h>

#include "Database.hpp"
#include "Logger.hpp"
#include "Utils.hpp"

void repl(Database &db);

auto main(const int argc, char *argv[]) -> int {
    int log_level = 0;
    for (int i = 1; i < argc; ++i) {
        if (std::string arg = argv[i]; arg.rfind("--log-level=", 0) == 0) {
            try {
                std::string level_str = arg.substr(14);
                log_level = std::stoi(level_str);
                if (log_level < 0) {
                    throw std::invalid_argument("Trace level cannot be negative.");
                }
            } catch (const std::exception &e) {
                std::cerr << "Invalid trace level value. It should be either 0 or 1. Instead it is: " << arg <<
                        std::endl;
                return 1;
            }
        }
    }
    Logger::set_log_level(log_level);

    const auto db_config = DatabaseConfig(100);
    auto db = Database(db_config);
    repl(db);

    return EXIT_SUCCESS;
}

void display_help();


void repl(Database &db) {
    namespace rg = std::ranges;

    fmt::println("EdgyDB v1.0.0");
    fmt::println("Type 'help' for list of options.");
    fmt::println("Type 'exit' or 'quit' to exit and save database.");

    auto const exit_commands = std::vector<std::string>{"exit", "quit"};

    while (true) {
        fmt::print("> ");
        std::string command;
        std::getline(std::cin, command);
        command = Utils::remove_consecutive_spaces(command);
        if (rg::contains(exit_commands, command)) {
            break;
        }

        if (command == "help") {
            display_help();
            continue;
        }
        if (auto query = Query::from_string(command); query.has_value()) {
            db.execute_query(query.value());
        }
    }
}

void display_help() {
    std::println("EdgyDB HELPER:");
    std::println("--------------------------------------------------");
    std::println("General Commands:");
    std::println("  USE [name]");
    std::println("    - Selects the graph to operate on. Example: USE firefighters");
    std::println("  CREATE GRAPH [name]");
    std::println("    - Creates a new graph. Example: CREATE GRAPH firefighters");

    std::println("\nNode Commands:");
    std::println("  INSERT NODE [data]");
    std::println("    - Adds a node with primitive data. Example: INSERT NODE \"Mariusz\"");
    std::println("  INSERT NODE COMPLEX [JSON]");
    std::println("    - Adds a node with user-defined structured data.");
    std::println(R"(      Example: INSERT NODE COMPLEX {{"name":"worker", "age":40, "salary":1000}})");
    std::println("  UPDATE NODE [node.id] TO [data]");
    std::println("    - Updates a node with primitive data. Example: UPDATE NODE 1 TO \"Krzysztof\"");
    std::println("  UPDATE NODE [node.id] TO COMPLEX [JSON]");
    std::println("    - Updates a node with user-defined structured data.");
    std::println(R"(      Example: UPDATE NODE 1 TO COMPLEX {{"name":"manager", "level":3}})");
    std::println("  SELECT NODE [node.id]");
    std::println("    - Displays data for a specific node. Example: SELECT NODE 1");
    std::println("  SELECT NODE WHERE [field] EQ/NEQ [value]");
    std::println("    - Queries nodes that meet specified conditions.");
    std::println(R"(      Example: SELECT NODE WHERE "position" EQ "manager" AND "age" NEQ 40)");

    std::println("\nEdge Commands:");
    std::println("  INSERT EDGE FROM [node.id] TO [node.id]");
    std::println("    - Creates a connection between two nodes. Example: INSERT EDGE FROM 1 TO 2");

    std::println("\nQuery and Connection Commands:");
    std::println("  IS [node.id] CONNECTED TO [node.id]");
    std::println("    - Checks if there is any connection between two nodes.");
    std::println("      Example: IS 2 CONNECTED TO 3");
    std::println("  IS [node.id] CONNECTED DIRECTLY TO [node.id]");
    std::println("    - Checks if there is a direct connection between two nodes.");
    std::println("      Example: IS 2 CONNECTED DIRECTLY TO 3");

    std::println("\nOther Commands:");
    std::println("  HELP");
    std::println("    - Displays this help message.");
    std::println("  EXIT");
    std::println("    - Closes the application.");

    std::println("--------------------------------------------------");
}
