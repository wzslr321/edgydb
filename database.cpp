//
// Created by Wiktor Zając on 27/11/2024.
//

#include "database.hpp"

#include <fstream>
#include <iostream>
#include <__algorithm/ranges_find_if.h>

#include "deserialization.hpp"
#include "Seeder.hpp"
#include "serialization.hpp"
#include "fmt/compile.h"

namespace rg = std::ranges;

auto Query::from_string(const std::string &query) -> Query {
    const auto space_separated_view = query | std::views::split(' ') | std::ranges::to<std::vector<std::string> >();

    std::vector<std::unique_ptr<Command> > commands{};
    for (const auto &word: space_separated_view) {
        commands.push_back(std::make_unique<Command>(word));
    }
    return Query{std::move(commands)};
}

template<std::predicate<const Node &> Predicate>
auto Graph::find_nodes_where(Predicate predicate) -> std::vector<Node> {
    auto filtered_nodes = nodes | std::views::filter(predicate);
    return std::vector<Node>(filtered_nodes.begin(), filtered_nodes.end());
}

auto Database::init_commands() -> std::vector<Command> {
    constexpr auto available_commands_count = 3;

    std::vector<Command> commands;
    commands.reserve(available_commands_count);

    commands.emplace_back("SELECT");
    commands.emplace_back("WHERE");
    commands.emplace_back("CREATE");

    return std::move(commands);
}

Database::Database(const DatabaseConfig config) : config(config) {
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

    valid_commands = init_commands();
}

auto Database::get_graphs() -> std::unique_ptr<std::vector<Graph> > {
    return std::make_unique<std::vector<Graph> >(this->graphs);
}

auto Database::is_command_semantic_valid(const std::string &keyword, const std::string &next) -> bool {
    const auto matched_command = rg::find_if(valid_commands, [&keyword](const auto &command) {
        return keyword == command.keyword;
    });
    if (matched_command == valid_commands.end()) return false;

    return rg::find_if(matched_command->next, [&next](const auto &command) {
        return next == command->keyword;
    }) != matched_command->next.end();
}

auto Database::is_query_valid(const Query &query) -> bool {
    auto const is_query_valid = rg::find_if(query.commands, [this](const auto &command) {
        return this->is_command_semantic_valid(command->keyword, command->value);
    }) != query.commands.end();

    return is_query_valid;
}

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


auto Database::execute_query(const Query &query) -> QueryResult {
    /*
    if (!is_query_valid(query)) {
        return QueryResult("validation failure", OperationResultStatus::SyntaxError, std::nullopt);
    }
    */

    if (query.commands.empty()) {
        return QueryResult("commands are empty", OperationResultStatus::ValueError, std::nullopt);
    }
    const auto &commands = query.commands;

    if (const auto &command = commands.front(); command->keyword == "SELECT") {
        const auto found = this->graphs[0].find_nodes_where([](const auto &node) {
            return node.id > 1;
        });
        auto result = QueryResultData{std::make_unique<std::vector<Node> >(found)};

        return QueryResult("Success", OperationResultStatus::Success, std::move(result));
    } else if (command->keyword == "WHERE") {
    }

    this->unsynchronized_queries_count += 1;
    if (this->unsynchronized_queries_count >= this->config.unsynced_queries_limit) {
        switch (const auto result = sync_with_storage(); result.status) {
            case IOResultStatus::Success:
                this->unsynchronized_queries_count = 0;
                break;
            default:
                return QueryResult("Failed to synchronize storage.Error: " + result.message,
                                   OperationResultStatus::ExecutionError, std::nullopt);
        }
    }

    return QueryResult("Success", OperationResultStatus::Success, std::nullopt);
}

