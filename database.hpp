//
// Created by Wiktor Zając on 27/11/2024.
//
#ifndef GRAPH_HPP
#define GRAPH_HPP

#include <string>
#include <variant>
#include <vector>
#include <ranges>
#include <sstream>

#include "Logger.hpp"
#include "fmt/core.h"

class Database;

struct BasicValue {
    std::variant<int, double, bool, std::string> data;

    [[nodiscard]] std::string toString() const {
        return std::visit([]<typename T0>(const T0 &arg) -> std::string {
            using T = std::decay_t<T0>;
            if constexpr (std::same_as<T, bool>) {
                return arg ? "true" : "false";
            } else if constexpr (std::same_as<T, std::string>) {
                return arg;
            } else {
                return std::to_string(arg);
            }
        }, this->data);
    }
};

struct UserDefinedValue {
private:
    std::vector<std::pair<std::string, std::variant<BasicValue, UserDefinedValue> > > data;

    static void validate_data(
        const std::vector<std::pair<std::string, std::variant<BasicValue, UserDefinedValue> > > &data) {
        const auto contains_name = std::ranges::find_if(data, [](const auto &pair) {
            return pair.first == "name";
        }) != data.end();
        if (!contains_name) {
            throw std::runtime_error("UserDefinedValue must have name specified");
        }
    }

public:
    UserDefinedValue() = default;

    ~UserDefinedValue() = default;

    explicit UserDefinedValue(
        const std::vector<std::pair<std::string, std::variant<BasicValue, UserDefinedValue> > > &data) : data(data) {
        validate_data(data);
        this->data = data;
    }

    void set_data(const std::vector<std::pair<std::string, std::variant<BasicValue, UserDefinedValue> > > &data) {
        validate_data(data);
        this->data = data;
    }

    [[nodiscard]] const std::vector<std::pair<std::string, std::variant<BasicValue, UserDefinedValue> > > &
    get_data() const {
        return data;
    }

    [[nodiscard]] std::string toString() const {
        std::ostringstream oss;
        oss << "{ ";
        for (size_t i = 0; i < data.size(); ++i) {
            const auto &[key, value] = data[i];
            oss << "\"" << key << "\": ";
            std::visit(
                [&oss](const auto &v) {
                    oss << v.toString();
                },
                value
            );
            if (i + 1 < data.size()) {
                oss << ", ";
            }
        }
        oss << " }";
        return oss.str();
    }
};

namespace rg = std::ranges;

struct Node {
    int id{};
    std::variant<BasicValue, UserDefinedValue> data;

    // TODO: ogarnij te pojebane to stringi
    [[nodiscard]] std::string toString() const {
        std::ostringstream oss;
        oss << "Node { id: " << id << ", data: ";

        std::visit(
            [&oss]<typename T0>(const T0 &value) {
                using T = std::decay_t<T0>;
                if constexpr (std::is_same_v<T, BasicValue>) {
                    oss << value.toString();
                } else if constexpr (std::is_same_v<T, UserDefinedValue>) {
                    oss << "{ ";
                    for (const auto &[key, sub_value]: value.get_data()) {
                        oss << key << ": ";
                        std::visit(
                            [&oss](const auto &sub) {
                                oss << sub.toString();
                            },
                            sub_value
                        );
                        oss << ", ";
                    }
                    oss << "}";
                }
            },
            data
        );

        oss << " }";
        return oss.str();
    }
};

struct Edge {
    int from{};
    int to{};
    // TODO:: possibly remove
    std::string relation;
};


struct Graph {
    std::string name;

    std::vector<Node> nodes;
    std::vector<Edge> edges;

    template<std::predicate<const Node &> Predicate>
    [[nodiscard]] std::vector<std::reference_wrapper<Node> > find_nodes_where(Predicate predicate);
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

struct Command {
    std::vector<std::unique_ptr<Command> > next{};
    std::string keyword;
    std::string value;

    explicit Command(std::string keyword, std::string value) : keyword(std::move(keyword)), value(std::move(value)) {
    }
};

class Query {
    std::vector<Command> commands{};

    static Logger logger;

    explicit Query(std::vector<Command> commands);

    auto handle_use(Database &db) const -> void;

    auto handle_create_graph(Database &db) const -> void;

    auto handle_insert_node(Database &db) const -> void;

    auto handle_insert_complex_node(Database &db) const -> void;

    auto handle_insert_edge(Database &db) const -> void;

    auto handle_select(Database &db) const -> void;

    auto handle_update_node(Database &db, bool isComplex) const -> void;

    auto handle_is_connected(Database &db, bool direct) const -> void;

    auto handle_select_node(Database &db) -> Node &;

public:
    auto handle(Database &db) const -> void;

    [[nodiscard]] auto get_commands() const -> const std::vector<Command> &;

    static auto from_string(const std::string &query) -> std::optional<Query>;
};

class Database {
    DatabaseConfig config{};

    std::vector<Graph> graphs{};
    Graph *current_graph = nullptr;

    static Logger logger;

    int unsynchronized_queries_count = 0;

    auto sync_with_storage() -> IOResult;

public:
    int current_id = 0;

    explicit Database(DatabaseConfig config);

    ~Database();

    auto execute_query(const Query &query) -> void;

    auto get_graph() const -> Graph &;

    auto get_graphs() -> std::vector<Graph> &;

    auto add_graph(Graph &graph) -> void;

    auto set_graph(Graph &graph) -> void;

    auto add_node(Node &node) const -> void;

    auto add_edge(Edge &edge) const -> void;

    auto update_node(Node &node) -> void;

    // auto find_node(const int id) -> std::optional<Node &>;
};


#endif //GRAPH_HPP
