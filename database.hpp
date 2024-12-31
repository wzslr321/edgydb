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

using BasicValue = std::variant<int, double, bool, std::string, std::vector<int>, std::vector<std::string>, std::vector<
    bool> >;

namespace rg = std::ranges;

struct UserDefinedValue {
    std::vector<std::pair<std::string, BasicValue> > values;
};

struct Node {
    int id{};
    std::string type;
    std::variant<BasicValue, UserDefinedValue> data;
};

struct Edge {
    int id{};
    int from{};
    int to{};
    std::string relation;
};

struct Graph {
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
};

#endif //GRAPH_HPP
