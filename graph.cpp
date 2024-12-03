//
// Created by Wiktor Zając on 27/11/2024.
//

#include "graph.hpp"

#include <__algorithm/ranges_find_if.h>

namespace rg = std::ranges;

auto Database::execute_query(const Query &query) -> OperationResult {
    auto const is_query_valid = rg::find_if(query.commands, [this](const auto &command) {
        return this->is_command_semantic_valid(command->keyword, command->value);
    }) != query.commands.end();
    if (!is_query_valid) {
        return {"Failure", false};
    }
    return {"Success", true};
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

auto Database::is_command_semantic_valid(const std::string &keyword, const std::string &next) -> bool {
    const auto matched_command = rg::find_if(valid_commands, [&keyword](const auto &command) {
        return keyword == command->keyword;
    });
    if (matched_command == valid_commands.end()) return false;
    return rg::find_if(matched_command->get()->next, [&next](const auto &command) {
        return next == command->keyword;
    }) != valid_commands.end();
}
