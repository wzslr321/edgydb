//
// Created by Wiktor Zając on 27/11/2024.
//

#include "database.hpp"

#include <fstream>
#include <iostream>
#include <ranges>

#include "deserialization.hpp"
#include "Seeder.hpp"
#include "serialization.hpp"

namespace rg = std::ranges;


template<std::predicate<const Node &> Predicate>
auto Graph::find_nodes_where(Predicate predicate) -> std::vector<Node> {
    auto filtered_nodes = nodes | std::views::filter(predicate);
    return std::vector<Node>(filtered_nodes.begin(), filtered_nodes.end());
}

auto Database::set_graph(Graph &graph) -> void {
    this->current_graph = &graph;
}

auto Database::seed() -> void {
    Seeder seeder;
    auto const graph = new Graph{};
    seeder.seed_graph(*graph);
    this->current_graph = graph;
}

Database::Database(const DatabaseConfig config) : config(config) {
    if (config.from_seed) {
        this->seed();
        return;
    }

    try {
        if (std::ifstream file("database_snapshot.json", std::ios::binary); file.is_open()) {
            std::ostringstream buffer;
            buffer << file.rdbuf();
            std::string json = buffer.str();
            std::erase_if(json, [](unsigned char c) {
                return std::isspace(c);
            });

            this->graphs = std::move(Deserialization::parse_graphs(json));
            std::cout << "Database successfully restored from file." << std::endl;
        } else {
            std::cerr << "No snapshot file found. Starting with an empty database." << std::endl;
        }
    } catch (const std::exception &e) {
        std::cerr << "Error during database restoration: " << e.what() << std::endl;
        std::cerr << "Starting with an empty database." << std::endl;
    }
}

auto Database::get_graph() const -> const Graph & {
    return *this->current_graph;
}

auto Database::get_graphs() -> std::vector<Graph> & {
    return this->graphs;
}

/*
auto Database::is_command_semantic_valid(const std::string &keyword, const std::string &next) -> bool {
    const auto matched_command = rg::find_if(valid_commands, [&keyword](const auto &command) {
        return keyword == command.keyword;
    });
    if (matched_command == valid_commands.end()) return false;

    return rg::find_if(matched_command->next, [&next](const auto &command) {
        return next == command->keyword;
    }) != matched_command->next.end();
}
*/

/*
auto Database::is_query_valid(const Query &query) -> bool {
    const auto &commands = query.get_commands();
    auto const is_query_valid = rg::find_if(commands, [this](const auto &command) {
        return this->is_command_semantic_valid(command.keyword, command.value);
    }) != commands.end();

    return is_query_valid;
}
*/

// TODO: Maybe just throw instead of IOResult
auto Database::sync_with_storage() -> IOResult {
    try {
        std::ofstream file("database_snapshot.json", std::ios::binary);
        if (!file.is_open()) {
            return IOResult("Failed to open file for writing", IOResultStatus::Failure);
        }

        file << Serialization::serialize_database(*this);
        file.close();

        unsynchronized_queries_count = 0;
        return IOResult("Database successfully saved to file", IOResultStatus::Success);
    } catch (const std::exception &e) {
        return IOResult(std::string("Error during sync: ") + e.what(), IOResultStatus::Failure);
    }
}


auto Database::execute_query(const Query &query) -> void {
    /*
    if (!is_query_valid(query)) {
        return QueryResult("validation failure", OperationResultStatus::SyntaxError, std::nullopt);
    }
    */

    /*
    if (const auto &commands = query.get_commands(); commands.empty()) {
        std::cerr << "No commands found. Nothing is executed." << std::endl;
        return;
    }
    */

    query.handle(*this);

    this->unsynchronized_queries_count += 1;
    if (this->unsynchronized_queries_count >= this->config.unsynced_queries_limit) {
        switch (const auto result = sync_with_storage(); result.status) {
            case IOResultStatus::Success:
                this->unsynchronized_queries_count = 0;
                break;
            default:
                std::cerr << std::format("Failed to synchronize storage. Error: {}", result.message);
        }
    }
}

Query::Query(std::vector<Command> commands) : commands(std::move(commands)) {
}

auto Query::from_string(const std::string &query) -> Query {
    const auto words = query | std::views::split(' ') | std::ranges::to<std::vector<std::string> >();

    std::vector<Command> commands{};
    for (int i = 0, size = words.size(); i < size - 1; i += 2) {
        commands.emplace_back(Command(words[i], words[i + 1]));
    }
    if (commands.empty()) {
        throw std::invalid_argument("Query can not be empty");
    }
    return Query(std::move(commands));
}

auto Query::handle_use(Database &db) const -> void {
    logger.info("Starting handle_use.");
    auto &graphs = db.get_graphs();
    logger.info(std::format("Graphs size: {}", graphs.size()));

    if (commands.empty()) {
        logger.error("Query commands are empty.");
        throw std::runtime_error("Query has no commands.");
    }

    const auto &command_value = this->get_commands().front().value;
    logger.info(std::format("Searching for graph with name: {}", command_value));

    const auto it = std::ranges::find_if(graphs, [&command_value](const auto &graph) {
        return graph.name == command_value;
    });

    if (it != graphs.end()) {
        logger.info(std::format("Graph found: {}", it->name));
        db.set_graph(*it);
    } else {
        logger.error("Graph not found.");
        throw std::runtime_error("Graph not found.");
    }
}

auto Query::handle(Database &db) const -> void {
    logger.info("STARTED HANDLE");
    handle_use(db);
    /*
    const auto &first_command = commands.front();
    logger.debug(std::format("Started attempt to handle query with first command: {}", first_command.keyword));
    if (first_command.keyword == "USE") {
        return handle_use(db);
    }
    throw std::invalid_argument("Unknown command");
    */
}

auto Query::get_commands() const -> const std::vector<Command> & {
    return commands;
}

Logger Database::logger("Database");
Logger Query::logger("Query");
