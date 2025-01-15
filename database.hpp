//
// Created by Wiktor Zając on 27/11/2024.
//
#ifndef GRAPH_HPP
#define GRAPH_HPP

#include <string>
#include <variant>
#include <vector>
#include <ranges>
#include "fmt/core.h"

struct BasicValue {
    std::variant<int, double, bool, std::string> data;

    [[nodiscard]] std::string toString() const {
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

struct UserDefinedValue {
    std::vector<std::pair<std::string, BasicValue> > data;
};

namespace rg = std::ranges;

struct Node {
    int id{};
    // TODO: Possibly removable type - I don't see a point
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
    std::string name;

    std::vector<Node> nodes;
    std::vector<Edge> edges;

    template<std::predicate<const Node &> Predicate>
    [[nodiscard]] std::vector<Node> find_nodes_where(Predicate predicate);
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


using QueryResultData = std::optional<std::variant<std::unique_ptr<std::vector<Edge> >, std::unique_ptr<std::vector<
    Node> > > >;

struct QueryResult {
    std::string message;
    OperationResultStatus status;
    QueryResultData data;

    explicit QueryResult(std::string message, const OperationResultStatus &status,
                         QueryResultData data)
        : message(std::move(message)), status(status), data(std::move(data)) {
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
    std::vector<Command> valid_commands{};

    int unsynchronized_queries_count = 0;

    static auto init_commands() -> std::vector<Command>;

    auto is_command_semantic_valid(const std::string &keyword, const std::string &next) -> bool;

    auto is_query_valid(const Query &query) -> bool;

    auto sync_with_storage() -> IOResult;

public:
    QueryResult execute_query(const Query &query);

    explicit Database(DatabaseConfig config);

    std::unique_ptr<std::vector<Graph> > get_graphs();
};

#endif //GRAPH_HPP
