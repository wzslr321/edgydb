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
};

enum OperationResultStatus {
    Success,
    SyntaxError,
    ValueError,
    ExecutionError,
};

struct OperationResult {
    std::string message;
    OperationResultStatus status;

    explicit OperationResult(std::string message, const OperationResultStatus &status)
        : message(std::move(message)), status(status) {
    }
};

class Database {
private:
    std::vector<Graph> graphs{};
    std::vector<std::unique_ptr<Command> > valid_commands{};

    static auto init_commands() -> std::vector<std::unique_ptr<Command> >;

    auto is_command_semantic_valid(const std::string &keyword, const std::string &next) -> bool;

public:
    explicit Database();

    OperationResult execute_query(const Query &query);
};

#endif //GRAPH_HPP
