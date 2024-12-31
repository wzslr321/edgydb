//
// Created by Wiktor Zając on 27/11/2024.
//

#include "database.hpp"

#include <__algorithm/ranges_find_if.h>

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
    // TODO: SAVE TO FILE

    unsynchronized_queries_count = 0;

    return IOResult("Success", IOResultStatus::Success);
}


auto Database::execute_query(const Query &query) -> QueryResult {
    if (!is_query_valid(query)) {
        return QueryResult("Failure", OperationResultStatus::SyntaxError);
    }
    this->unsynchronized_queries_count += 1;

    return QueryResult("Success", OperationResultStatus::Success);
}
