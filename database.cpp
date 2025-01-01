//
// Created by Wiktor Zając on 27/11/2024.
//

#include "database.hpp"

#include <iostream>
#include <__algorithm/ranges_find_if.h>

#include "serialization.hpp"

namespace rg = std::ranges;

auto Query::from_string(const std::string &query) -> Query {
    const auto space_separated_view = query | std::views::split(' ') | std::ranges::to<std::vector<std::string> >();

    std::vector<std::unique_ptr<Command> > commands{};
    for (const auto &word: space_separated_view) {
        commands.push_back(std::make_unique<Command>(word));
    }
    return Query{std::move(commands)};
}

auto Database::init_commands() -> std::vector<std::unique_ptr<Command> > {
    constexpr auto available_commands_count = 3;

    std::vector<std::unique_ptr<Command> > commands;
    commands.reserve(available_commands_count);

    commands.push_back(std::make_unique<Command>("SELECT"));
    commands.push_back(std::make_unique<Command>("WHERE"));
    commands.push_back(std::make_unique<Command>("CREATE"));

    return commands;
}

Database::Database(const DatabaseConfig config) : config(std::move(config)) {
    valid_commands = init_commands();

    // First graph
    Graph graph1;

    // Add nodes to the first graph
    graph1.nodes.push_back({1, "Person", BasicValue{"Alice"}});
    graph1.nodes.push_back({2, "Person", BasicValue{"Bob"}});
    graph1.nodes.push_back({3, "City", BasicValue{"New York"}});

    // Add edges to the first graph
    graph1.edges.push_back({1, 1, 2, "knows"}); // Alice knows Bob
    graph1.edges.push_back({2, 1, 3, "lives_in"}); // Alice lives in New York
    graph1.edges.push_back({3, 2, 3, "visited"}); // Bob visited New York

    // Second graph
    Graph graph2;

    // Add nodes to the second graph
    graph2.nodes.push_back({4, "Company", BasicValue{"TechCorp"}});
    graph2.nodes.push_back({5, "Employee", BasicValue{"Charlie"}});
    graph2.nodes.push_back({6, "Employee", BasicValue{"Diana"}});

    // Add edges to the second graph
    graph2.edges.push_back({4, 5, 4, "works_at"}); // Charlie works at TechCorp
    graph2.edges.push_back({5, 6, 4, "colleague"}); // Diana is a colleague of Charlie
    graph2.edges.push_back({6, 4, 5, "founded"}); // TechCorp was founded by Diana

    // Add graphs to the database
    graphs.push_back(graph1);
    graphs.push_back(graph2);
}

auto Database::is_command_semantic_valid(const std::string &keyword, const std::string &next) -> bool {
    const auto matched_command = rg::find_if(valid_commands, [&keyword](const auto &command) {
        return keyword == command->keyword;
    });
    if (matched_command == valid_commands.end()) return false;
    return rg::find_if(matched_command->get()->next, [&next](const auto &command) {
        return next == command->keyword;
    }) != valid_commands.end();
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

        file << "{";
        file << "\"graphs\":[";
        for (size_t i = 0; i < graphs.size(); ++i) {
            file << Serialization::serialize_graph(graphs[i]);
            if (i + 1 < graphs.size()) file << ",";
        }
        file << "]";
        file << "}";
        file.close();

        unsynchronized_queries_count = 0;
        return IOResult("Database successfully saved to file", IOResultStatus::Success);
    } catch (const std::exception &e) {
        return IOResult(std::string("Error during sync: ") + e.what(), IOResultStatus::Failure);
    }
}


auto Database::execute_query(const Query &query) -> QueryResult {
    if (!is_query_valid(query)) {
        return QueryResult("Failure", OperationResultStatus::SyntaxError);
    }

    this->unsynchronized_queries_count += 1;
    if (this->unsynchronized_queries_count >= this->config.unsynced_queries_limit) {
        switch (const auto result = sync_with_storage(); result.status) {
            case IOResultStatus::Success:
                this->unsynchronized_queries_count = 0;
                break;
            default:
                return QueryResult("Failed to synchronize storage.Error: " + result.message,
                                   OperationResultStatus::ExecutionError);
        }
    }

    return QueryResult("Success", OperationResultStatus::Success);
}
