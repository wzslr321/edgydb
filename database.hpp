//
// Created by Wiktor Zając on 27/11/2024.
//
#pragma once

#ifndef GRAPH_HPP
#define GRAPH_HPP

#include <string>
#include <variant>
#include <vector>
#include <ranges>
#include <fstream>

#include "database.hpp"
#include "fmt/core.h"

struct BasicValue {
    std::variant<int, double, bool, std::string> data;

    std::string toString() const {
        return std::visit([]<typename T0>(const T0 &arg) -> std::string {
            using T = std::decay_t<T0>;
            if constexpr (std::is_same_v<T, bool>) {
                return arg ? "true" : "false";
            } else if constexpr (std::is_same_v<T, std::string>) {
                return arg;
            } else {
                return std::to_string(arg);
            }
        }, this->data);
    }
};

namespace rg = std::ranges;

struct Node {
    int id{};
    std::string type;
    BasicValue data;
};

struct Edge {
    int id{};
    int from{};
    int to{};
    std::string relation;
};

struct Graph {
    std::string name;

    std::vector<Node> nodes;
    std::vector<Edge> edges;
};


struct Command {
    std::vector<std::unique_ptr<Command> > next{};
    std::string keyword;
    std::string value;

    explicit Command(std::string keyword) : keyword(std::move(keyword)) {
    }
};

struct Query {
    std::vector<std::unique_ptr<Command> > commands{};

    static auto from_string(const std::string &query) -> Query;

    explicit Query(std::vector<std::unique_ptr<Command> > commands)
        : commands(std::move(commands)) {
    };
};

enum class OperationResultStatus {
    Success,
    SyntaxError,
    ValueError,
    ExecutionError,
};

struct QueryResult {
    std::string message;
    OperationResultStatus status;

    explicit QueryResult(std::string message, const OperationResultStatus &status)
        : message(std::move(message)), status(status) {
    }
};

enum class IOResultStatus {
    Success,
    Failure,
};

struct IOResult {
    std::string message;
    IOResultStatus status;

    explicit IOResult(std::string message, const IOResultStatus status): message(std::move(message)),
                                                                         status(status) {
    }
};

struct DatabaseConfig {
    int unsynced_queries_limit{};

    explicit DatabaseConfig(const int unsynced_queries_limit = 10) : unsynced_queries_limit(
        unsynced_queries_limit) {
    }
};

class Database {
    DatabaseConfig config{};

    std::vector<Graph> graphs{};
    std::vector<std::unique_ptr<Command> > valid_commands{};

    int unsynchronized_queries_count = 0;

    static auto init_commands() -> std::vector<std::unique_ptr<Command> >;

    auto is_command_semantic_valid(const std::string &keyword, const std::string &next) -> bool;

    auto is_query_valid(const Query &query) -> bool;

    auto sync_with_storage() -> IOResult;

public:
    QueryResult execute_query(const Query &query);

    explicit Database(const DatabaseConfig config);

    std::vector<std::unique_ptr<Graph> > get_graphs();
};

#endif //GRAPH_HPP
