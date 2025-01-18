//
// Created by Wiktor Zając on 27/11/2024.
//

#include "database.hpp"

#include <fstream>
#include <iostream>
#include <ranges>
#include <algorithm>
#include <numeric>
#include <unordered_set>

#include "condition.hpp"
#include "deserialization.hpp"
#include "serialization.hpp"
#include "Utils.hpp"
#include "fmt/compile.h"

namespace rg = std::ranges;


template<std::predicate<const Node &> Predicate>
auto Graph::find_nodes_where(Predicate predicate) -> std::vector<std::reference_wrapper<Node> > {
    std::vector<std::reference_wrapper<Node> > result;

    for (auto &node: nodes) {
        if (predicate(node)) {
            result.emplace_back(node);
        }
    }

    return result;
}


auto Database::set_graph(Graph &graph) -> void {
    this->current_graph = &graph;
}

auto Database::add_node(Node &node) const -> void {
    if (this->current_graph == nullptr) {
        std::cerr << "To execute queries first specify graph with USE command" << std::endl;
        return;
    }
    logger.info(std::format("Adding node with id {} to the graph with name {}", node.id, this->current_graph->name));
    this->current_graph->nodes.emplace_back(node);
}

auto Database::add_edge(Edge &edge) const -> void {
    if (this->current_graph == nullptr) {
        std::cerr << "To execute queries first specify graph with USE command" << std::endl;
        return;
    }
    logger.info(std::format("Adding edge from {} to {}", edge.from, edge.to));
    this->current_graph->edges.emplace_back(edge);
}

Database::Database(const DatabaseConfig config) : config(config) {
    try {
        if (std::ifstream file("database_snapshot.json"); file.is_open()) {
            std::ostringstream buffer;
            buffer << file.rdbuf();
            std::string json = buffer.str();
            std::erase_if(json, [](unsigned char c) {
                return std::isspace(c);
            });

            this->graphs = std::move(Deserialization::parse_graphs(json));
            std::cout << "Database successfully restored from file." << std::endl;
        } else {
            std::cerr << "No snapshot file found. Starting with an empty database." << std::endl;
        }
    } catch (const std::exception &e) {
        std::cerr << "Error during database restoration: " << e.what() << std::endl;
        std::cerr << "Starting with an empty database." << std::endl;
    }
}

auto Database::get_graph() const -> Graph & {
    return *this->current_graph;
}

auto Database::get_graphs() -> std::vector<Graph> & {
    return this->graphs;
}

auto Database::add_graph(Graph &graph) -> void {
    auto const graph_already_exists = std::ranges::find_if(this->graphs, [&graph](const Graph &g) {
        return g.name == graph.name;
    }) != this->graphs.end();
    if (graph_already_exists) {
        std::cerr << "Graph " << graph.name << " already exists." << std::endl;
    } else {
        logger.info(std::format("Created new graph with name {}", graph.name));
        this->graphs.push_back(graph);
    }
}

// TODO: Maybe just throw instead of IOResult
auto Database::sync_with_storage() -> IOResult {
    try {
        std::ofstream file("database_snapshot.json", std::ios::binary);
        if (!file.is_open()) {
            return IOResult("Failed to open file for writing", IOResultStatus::Failure);
        }

        file << Serialization::serialize_database(*this);
        file.close();

        unsynchronized_queries_count = 0;
        return IOResult("Database successfully saved to file", IOResultStatus::Success);
    } catch (const std::exception &e) {
        return IOResult(std::string("Error during sync: ") + e.what(), IOResultStatus::Failure);
    }
}

Database::~Database() {
    logger.info("Attempting to synchronize database before closing");
    switch (const auto result = sync_with_storage(); result.status) {
        case IOResultStatus::Success:
            this->unsynchronized_queries_count = 0;
            break;
        default:
            std::cerr << std::format("Failed to synchronize storage. Error: {}", result.message);
    }
}


auto Database::execute_query(const Query &query) -> void {
    query.handle(*this);

    this->unsynchronized_queries_count += 1;
    if (this->unsynchronized_queries_count >= this->config.unsynced_queries_limit) {
        switch (const auto result = sync_with_storage(); result.status) {
            case IOResultStatus::Success:
                this->unsynchronized_queries_count = 0;
                break;
            default:
                std::cerr << std::format("Failed to synchronize storage. Error: {}", result.message);
        }
    }
}

Query::Query(std::vector<Command> commands) : commands(std::move(commands)) {
}

auto Query::from_string(const std::string &query) -> std::optional<Query> {
    const auto words = query | std::views::split(' ') | std::ranges::to<std::vector<std::string> >();

    if (words.size() < 2) {
        throw std::invalid_argument("Query can not be empty");
    }

    std::vector<Command> commands{};
    if (words.size() == 2) {
        if (words[0] == "USE") {
            commands.emplace_back(words[0], words[1]);
            return Query(std::move(commands));
        }
        std::cerr << "Only USE command can have a single argument";
        return std::nullopt;
    }

    if (words.size() == 5 && words[0] == "IS" && words[2] == "CONNECTED" && words[3] == "TO") {
        commands.emplace_back("IS CONNECTED", words[1] + " " + words[4]);
        return Query(std::move(commands));
    }

    if (words.size() == 6 && words[0] == "IS" && words[2] == "CONNECTED" && words[3] == "TO" && words[4] ==
        "DIRECTLY") {
        commands.emplace_back("IS CONNECTED DIRECTLY", words[1] + " " + words[5]);
        return Query(std::move(commands));
    }

    if (words.size() == 3) {
        if (words[0] == "CREATE") {
            if (words[1] == "GRAPH") {
                commands.emplace_back("CREATE GRAPH", words[2]);
                return Query(std::move(commands));
            }
            std::cerr << "CREATE command only support GRAPH argument" << std::endl;
            return std::nullopt;
        }
        if (words[0] == "INSERT") {
            if (words[1] == "NODE" || words[1] == "EDGE") {
                commands.emplace_back(std::format("INSERT {}", words[1]), words[2]);
                return Query(std::move(commands));
            }
            throw std::invalid_argument("INSERT command support only NODE and EDGE");
        }
        if (words[0] == "UPDATE") {
            if (words[1] == "NODE") {
                commands.emplace_back("UPDATE NODE", words[2]);
                return Query(std::move(commands));
            }
            throw std::invalid_argument("UPDATE command support only NODE");
        }
        if (words[0] == "SELECT") {
            if (words[1] == "NODE") {
                commands.emplace_back("SELECT NODE", words[2]);
                return Query(std::move(commands));
            }
            throw std::invalid_argument("SELECT command support only NODE");
        }
        throw std::invalid_argument("Only CREATE, UPDATE, SELECT can consist of two arguments");
    }

    if (words.size() >= 4) {
        // ensure size checks as json spaces may conflict
        if (words[0] == "INSERT" && words[1] == "NODE" && words[2] == "COMPLEX") {
            const auto rest = Utils::get_rest_of_space_separated_string(words, 3);
            commands.emplace_back("INSERT NODE COMPLEX", Utils::minifyJson(rest));
            return Query(std::move(commands));
        }
        if (words.size() == 6 && words[0] == "INSERT" && words[1] == "EDGE" && words[2] == "FROM" && words[4] == "TO") {
            auto val = words[3] + " " + words[5];
            commands.emplace_back("INSERT EDGE FROM TO", val);
            return Query(std::move(commands));
        }
        if (words.size() == 5 && words[0] == "UPDATE" && words[1] == "NODE" && words[3] == "TO") {
            auto val = words[2] + " " + Utils::get_rest_of_space_separated_string(words, 4);
            commands.emplace_back("UPDATE NODE TO", val);
            return Query(std::move(commands));
        }
        if (words[0] == "UPDATE" && words[1] == "NODE" && words[3] == "TO" && words[4] ==
            "COMPLEX") {
            auto val = words[2] + " " + Utils::get_rest_of_space_separated_string(words, 5);
            commands.emplace_back("UPDATE NODE TO COMPLEX", val);
            return Query(std::move(commands));
        }


        std::ostringstream conditions_stream;
        for (size_t i = 3; i < words.size(); ++i) {
            if (words[i] == "AND" || words[i] == "OR" || words[i].starts_with("\"")) {
                conditions_stream << words[i] << " ";
            } else if (words[i] == "EQ" || words[i] == "NEQ") {
                conditions_stream << words[i] << " ";
            } else {
                throw std::invalid_argument("Unexpected token in SELECT NODE WHERE query");
            }
        }

        commands.emplace_back("SELECT NODE WHERE", Utils::trim(conditions_stream.str()));
        return Query(std::move(commands));
    }


    return Query(std::move(commands));
}

// TODO: remove this logger shit
auto Query::handle_use(Database &db) const -> void {
    logger.info("Starting handle_use.");
    auto &graphs = db.get_graphs();
    logger.info(std::format("Graphs size: {}", graphs.size()));

    if (commands.empty()) {
        logger.error("Query commands are empty.");
        throw std::runtime_error("Query has no commands.");
    }

    const auto &command_value = this->get_commands().front().value;
    logger.info(std::format("Searching for graph with name: {}", command_value));

    const auto it = std::ranges::find_if(graphs, [&command_value](const auto &graph) {
        return graph.name == command_value;
    });

    if (it != graphs.end()) {
        logger.info(std::format("Graph found: {}", it->name));
        db.current_id = it->nodes.size();
        db.set_graph(*it);
    } else {
        logger.error("Graph not found.");
        std::cerr << "Graph not found. If you want to create it, use CREATE GRAPH command" << std::endl;
    }
}

auto Query::handle_select_where(Database &db) const -> void {
    logger.debug("SELECT NODE WHERE started");
    const auto condition_str = this->commands.front().value;

    try {
        auto condition_group = parse_conditions(condition_str);
        auto &graph = db.get_graph();

        auto matches_conditions = [&condition_group](const Node &node) {
            auto evaluate_condition = [&node](const Condition &condition) {
                if (!std::holds_alternative<UserDefinedValue>(node.data)) {
                    return false;
                }
                const auto &user_data = std::get<UserDefinedValue>(node.data);
                const auto &fields = user_data.get_data();

                auto it = std::ranges::find_if(fields, [&condition](const auto &field) {
                    return field.first == condition.field;
                });

                if (it == fields.end()) return false;

                const auto &field_value = std::get<BasicValue>(it->second).toString();
                return (condition.comparator == "EQ" && field_value == condition.value) ||
                       (condition.comparator == "NEQ" && field_value != condition.value);
            };

            bool result = evaluate_condition(condition_group.conditions.front());
            for (size_t i = 0; i < condition_group.operators.size(); ++i) {
                const auto &op = condition_group.operators[i];
                const auto &condition = condition_group.conditions[i + 1];

                if (op == "AND") {
                    result = result && evaluate_condition(condition);
                } else if (op == "OR") {
                    result = result || evaluate_condition(condition);
                }
            }

            return result;
        };

        auto matching_nodes = graph.find_nodes_where(matches_conditions);

        if (matching_nodes.empty()) {
            std::cout << "No nodes matched the given conditions.\n";
        } else {
            std::cout << "Matching nodes:\n";
            for (const auto &node_ref: matching_nodes) {
                std::cout << node_ref.get().toString() << "\n";
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "Failed to process SELECT query: " << e.what() << "\n";
    }
}


auto Query::handle_create_graph(Database &db) const -> void {
    logger.debug("CREATE GRAPH started");
    Graph graph = {this->commands.front().value};
    db.add_graph(graph);
}

auto Query::handle_insert_node(Database &db) const -> void {
    logger.debug("INSERT NODE started");
    size_t pos = 0;
    auto value = Deserialization::parse_value(this->commands.front().value, pos);
    Node node = {++db.current_id, value};
    db.add_node(node);
}

auto Query::handle_insert_complex_node(Database &db) const -> void {
    logger.debug("INSERT NODE COMPLEX started");
    try {
        size_t pos = 0;
        auto value = Deserialization::parse_user_defined_value(this->commands.front().value, pos);
        Node node = {++db.current_id, value};
        db.add_node(node);
    } catch (std::runtime_error &e) {
        std::cerr << "Failed to parse query. Value is not a proper JSON" << std::endl;
    }
}

auto Query::handle_insert_edge(Database &db) const -> void {
    logger.debug("INSERT EDGE started");
    auto command = this->commands.front().value;
    const auto node_ids = command | std::views::split(' ') | std::ranges::to<std::vector<std::string> >();
    try {
        const auto from_id = std::stoi(node_ids[0]);
        const auto to_id = std::stoi(node_ids[1]);
        Edge edge = {from_id, to_id};
        db.add_edge(edge);
    } catch (std::invalid_argument &e) {
        std::cerr << "Failed to insert edge. Node id is not valid integer" << std::endl;
    }
}

auto Query::handle_select(Database &db) const -> void {
    logger.debug("SELECT NODE started");
    const auto command = this->commands.front().value;
    try {
        auto id = std::stoi(command);
        auto nodes = db.get_graph().find_nodes_where([&](const auto &node) {
            return node.id == id;
        });
        if (nodes.empty()) {
            std::cerr << std::format("No node found with id {}", id) << std::endl;
        } else {
            fmt::print("Found node with id {}\n{}\n", nodes.size(), nodes.front().get().toString());
        }
    } catch (std::invalid_argument &e) {
        std::cerr << "Failed to select node. Node id is not valid integer" << std::endl;
    }
}

auto Query::handle_update_node(Database &db, bool isComplex) const -> void {
    logger.debug("UPDATE NODE started");
    auto value = this->commands.front().value;
    auto parts = value | std::views::split(' ') | std::ranges::to<std::vector<std::string> >();
    auto new_value = Utils::get_rest_of_space_separated_string(parts, 1);
    try {
        const auto node_id = std::stoi(parts[0]);
        Node *matched_node = nullptr;
        for (auto &node: db.get_graph().nodes) {
            if (node_id == node.id) {
                matched_node = &node;
            }
        }
        if (matched_node != nullptr) {
            size_t pos = 0;
            std::variant<BasicValue, UserDefinedValue> value;
            if (isComplex) {
                value = Deserialization::parse_user_defined_value(
                    Utils::minifyJson(new_value), pos);
            } else {
                value = Deserialization::parse_value(new_value, pos);
            }
            matched_node->data = value;
            logger.info(std::format("Successfully updated node with id {}", node_id));
        } else {
            std::cerr << "Update failed. No node found with given id" << std::endl;
        }
    } catch (std::invalid_argument &e) {
        std::cerr << "Failed to update node. Node id is not valid integer" << std::endl;
    }
}

auto Query::handle_is_connected(Database &db, bool direct) const -> void {
    logger.debug("IS CONNECTED started");
    const auto command = this->commands.front().value;
    const auto node_ids = command | std::views::split(' ') | std::ranges::to<std::vector<std::string> >();

    try {
        const auto node1_id = std::stoi(node_ids[0]);
        const auto node2_id = std::stoi(node_ids[1]);

        auto &graph = db.get_graph();

        if (direct) {
            bool connected = std::ranges::any_of(graph.edges, [&](const Edge &edge) {
                return (edge.from == node1_id && edge.to == node2_id) ||
                       (edge.from == node2_id && edge.to == node1_id);
            });

            fmt::print("Nodes {} and {} are {}directly connected.\n",
                       node1_id, node2_id, connected ? "" : "not ");
        } else {
            std::unordered_set<int> visited;
            std::queue<int> to_visit;
            to_visit.push(node1_id);

            bool connected = false;
            while (!to_visit.empty()) {
                int current = to_visit.front();
                to_visit.pop();

                if (current == node2_id) {
                    connected = true;
                    break;
                }

                if (visited.contains(current)) {
                    continue;
                }

                visited.insert(current);
                for (const auto &edge: graph.edges) {
                    if (edge.from == current && !visited.contains(edge.to)) {
                        to_visit.push(edge.to);
                    }
                    if (edge.to == current && !visited.contains(edge.from)) {
                        to_visit.push(edge.from);
                    }
                }
            }

            fmt::print("Nodes {} and {} are {}connected.\n",
                       node1_id, node2_id, connected ? "" : "not ");
        }
    } catch (std::invalid_argument &) {
        std::cerr << "Failed to parse node IDs. Ensure they are valid integers.\n";
    }
}

auto Query::handle(Database &db) const -> void {
    const auto &first_command = commands.front();
    logger.debug(std::format("Started attempt to handle query with first command: {}", first_command.keyword));
    if (first_command.keyword == "USE") {
        return handle_use(db);
    }
    if (first_command.keyword == "CREATE GRAPH") {
        return handle_create_graph(db);
    }
    if (first_command.keyword == "INSERT NODE") {
        return handle_insert_node(db);
    }
    if (first_command.keyword == "INSERT NODE COMPLEX") {
        return handle_insert_complex_node(db);
    }
    if (first_command.keyword == "INSERT EDGE") {
        return handle_insert_edge(db);
    }
    if (first_command.keyword == "UPDATE NODE TO") {
        return handle_update_node(db, false);
    }
    if (first_command.keyword == "UPDATE NODE TO COMPLEX") {
        return handle_update_node(db, true);
    }
    if (first_command.keyword == "SELECT NODE") {
        return handle_select(db);
    }
    if (first_command.keyword == "SELECT NODE WHERE") {
        return handle_select_where(db);
    }
    if (first_command.keyword == "INSERT EDGE FROM TO") {
        return handle_insert_edge(db);
    }
    if (first_command.keyword == "IS CONNECTED") {
        return handle_is_connected(db, false);
    }
    if (first_command.keyword == "IS CONNECTED DIRECTLY") {
        return handle_is_connected(db, true);
    }
    throw std::invalid_argument("Unknown command");
}

auto Query::get_commands() const -> const std::vector<Command> & {
    return commands;
}

Logger Database::logger("Database");
Logger Query::logger("Query");
