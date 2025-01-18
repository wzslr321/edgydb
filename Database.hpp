//
// Created by Wiktor Zając on 27/11/2024.
//
#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <string>
#include <variant>
#include <vector>
#include <ranges>
#include <sstream>
#include "fmt/core.h"

#include "Logger.hpp"

class Database;

namespace rg = std::ranges;


struct BasicValue {
    using Data = std::variant<int, double, bool, std::string>;
    Data data;

    // TODO: Simplify those awful toString
    [[nodiscard]]
    auto toString() const -> std::string {
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
    // TODO: Ensure uses everywhere
    using Data = std::vector<std::pair<std::string, std::variant<BasicValue, UserDefinedValue> > >;

private:
    Data data;

    static auto validate_data(const Data &data) -> void {
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

    explicit UserDefinedValue(Data data) {
        set_data(std::move(data));
    }

    auto set_data(Data data) -> void {
        validate_data(data);
        this->data = std::move(data);
    }

    [[nodiscard]]
    auto get_data() const -> Data const & {
        return data;
    }

    [[nodiscard]]
    auto toString() const -> std::string {
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


struct Node {
    int id{};
    using Data = std::variant<BasicValue, UserDefinedValue>;
    Data data;

    [[nodiscard]]
    auto toString() -> std::string {
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
};

struct Graph {
    std::string name;

    std::vector<Node> nodes;
    std::vector<Edge> edges;

    template<std::predicate<const Node &> Predicate>
    [[nodiscard]]
    auto find_nodes_where(Predicate predicate) -> std::vector<std::reference_wrapper<Node> >;
};

struct DatabaseConfig {
    int unsynced_queries_limit{};

    explicit DatabaseConfig(const int unsynced_queries_limit = 10) : unsynced_queries_limit(
        unsynced_queries_limit) {
    }
};

struct Command {
    std::string keyword;
    std::string value;

    explicit Command(std::string keyword, std::string value) : keyword(std::move(keyword)), value(std::move(value)) {
    }
};

class Query {
    std::vector<Command> commands{};

    inline static auto logger = Logger("Query");

    explicit Query(std::vector<Command> commands);

    auto handle_use(Database &db) const -> void;

    auto handle_select_where(const Database &db) const -> void;

    auto handle_create_graph(Database &db) const -> void;

    auto handle_insert_node(Database &db) const -> void;

    auto handle_insert_complex_node(Database &db) const -> void;

    auto handle_insert_edge(const Database &db) const -> void;

    auto handle_select(const Database &db) const -> void;

    auto handle_update_node(Database &db, bool isComplex) const -> void;

    auto handle_is_connected(const Database &db, bool direct) const -> void;

public:
    auto handle(Database &db) const -> void;

    [[nodiscard]] auto get_commands() const -> const std::vector<Command> &;

    static auto from_string(const std::string &query) -> std::optional<Query>;
};

class Database {
    DatabaseConfig config{};

    std::vector<Graph> graphs{};
    Graph *current_graph = nullptr;

    static inline auto logger = Logger("Database");

    int unsynchronized_queries_count = 0;

    auto sync_with_storage() -> void;

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
};

#endif //DATABASE_HPP
