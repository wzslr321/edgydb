#include <iostream>
#include <fmt/core.h>
#include <__algorithm/ranges_contains.h>

#include "database.hpp"

/// Structure of the database consists of nodes and edges, therefore it is a graph.
/// Database can consist of multiple graphs.
///   Node can directly hold any value, e.g. string.
/// Nodes also support complex types, although under the hood
/// it is stored in another place, and node only has a reference to it.
///   Edge 'connects' nodes, therefore is a relation between them.
/// The graph is directed, which means that edge A->B (where A->B syntax means edge from node A to node B)
/// is not the exact same edge as edge B->A
/// If there is an edge A->B, there must exist edge B->A
/// Edge must hold additional information (simply via name) about relation between nodes it connects
///
/// Language supporting above functionalities:
///
/// DDL (Data Definition Language)
///
/// CREATE TYPE Student                                     <--- DEFINE CUSTOM TYPE
///     | WITH FIELDS
///         | id INT,
///         | name STRING,
///         | email STRING
///
/// CREATE EDGE [EDGE_NAME_1, ..., EDGE_NAME_N]             <---  DEFINE EDGES
///     | WHERE EDGE [EDGE_NAME_1] CONNECTS                 <---  RESTRICT WHAT VALUES THE EDGE CAN CONNECT
///         | INT AND INT
///         | INT AND Student
///
/// CREATE NODE [NODE_NAME_1, ..., NODE_NAME_N]             <--- DEFINE NODES
///     | WHERE NODE [NODE_NAME_1] IS Student,
///     ...
///     | WHERE NODE [NODE_NAME_N] IS INT                   <--- SPECIFY NODE'S VALUE TYPE
///     | WHERE NODE [NODE_NAME_N] ACCEPTS OUT              <--- SPECIFY WHICH EDGES CAN START IN THE NODE
///         | EDGE_NAME_1
///     | WHERE NODE [NODE_NAME_N] ACCEPTS IN               <--- SPECIFY WHICH EDGES CAN END IN THE NODE
///         | EDGE_NAME_N
///
/// (NOTE): EDGES AND NODES CAN BE DEFINED INDEPENDENTLY,
/// AS THOSE CAN BE REUSED IN MULTIPLE GRAPHS
///
/// CREATE GRAPH [NAME]                                     <--- DEFINE GRAPH STRUCTURE
///     | WITH NODES [NODE_NAME_1, ..., NODE_NAME_N]
///
///
/// DML (Data Modification Language)
///
/// INSERT NODE [NODE_NAME] INTO [GRAPH_NAME]
///     | WITH VALUE "SOME VALUE"
///
/// UPDATE NODE [NODE_NAME] TO "SOME OTHER VALUE"
///
/// REMOVE NODE [NODE_NAME]
///
/// INSERT EDGE [EDGE_NAME]
///     | FROM [NODE_NAME_1]
///     | TO [NODE_NAME_2]
///
/// REMOVE EDGE [EDGE_NAME]
///
/// (NOTE): There can exist multiple nodes with the same value.
/// To handle that, internal ids are added to every node.
/// Those ids can be displayed via SELECT (more about it later) command
/// and then used via WHERE clause to narrow which node should be affected by given command
///
///
/// DQL (Data Query Language)
///
/// SELECT NODE
///     | WHERE VALUE > 50 && VALUE < 100
///     | WHERE COUNT(NODE.OUT) > 3
///
/// PATH FROM [NODE_NAME] TO [NODE_NAME]
///
/// TRAVERSE FROM [NODE_NAME]
///     | WITH DEPTH=3

void repl(Database &db);

auto main() -> int {
    const auto db_config = DatabaseConfig(1);
    auto db = Database(db_config);
    repl(db);
    return 0;
}

void display_help();

void repl(Database &db) {
    namespace rg = std::ranges;

    fmt::println("EdgyDB v1.0.0");
    fmt::println("Type 'help' for list of options.");

    auto const exit_commands = std::vector<std::string>{"exit", "quit"};

    while (true) {
        fmt::print("> ");
        std::string command;
        std::getline(std::cin, command);
        if (rg::contains(exit_commands, command)) break;
        if (command == "help") {
            display_help();
            continue;
        }
        auto result = db.execute_query(Query::from_string(command));

        fmt::println("{}", result.message);
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
