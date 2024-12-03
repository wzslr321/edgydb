//
// Created by Wiktor Zając on 03/12/2024.
//
#pragma once

#ifndef DATABASE_HPP
#define DATABASE_HPP
#include <string>
#include <vector>
#include <__algorithm/ranges_find_if.h>

namespace rg = std::ranges;

struct Command {
    std::vector<std::unique_ptr<Command> > next{};
    std::string keyword;
    std::string value;

    explicit Command(std::string keyword) : keyword(std::move(keyword)) {
    }
};

struct Query {
    std::vector<std::unique_ptr<Command> > commands{};
};

class Database {
private:
    std::vector<std::unique_ptr<Command> > valid_commands{};

    static auto init_commands() -> std::vector<std::unique_ptr<Command> > {
        auto select_command = std::make_unique<Command>("SELECT");
        auto where_command = std::make_unique<Command>("WHERE");
        auto create_command = std::make_unique<Command>("CREATE");

        return {std::move(select_command), std::move(where_command), std::move(create_command)};
    }

    auto is_command_semantic_valid(const std::string &keyword, const std::string &next) -> bool {
        auto matched_command = rg::find_if(valid_commands, [&keyword](const auto &command) {
            return keyword == command->keyword;
        });
        if (matched_command == valid_commands.end()) return false;
        return rg::find_if(matched_command->get()->next, [&next](const auto &command) {
            return next == command->keyword;
        }) != valid_commands.end();
    }

public:
    explicit Database() {
        valid_commands = init_commands();
    };

    ~Database();
};


#endif //DATABASE_HPP
