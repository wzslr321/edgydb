//
// Created by Wiktor Zając on 27/11/2024.
//

#include "database.hpp"

#include <fstream>
#include <iostream>
#include <ranges>
#include <algorithm>
#include <numeric>

#include "deserialization.hpp"
#include "Seeder.hpp"
#include "serialization.hpp"
#include "Utils.hpp"

namespace rg = std::ranges;


template<std::predicate<const Node &> Predicate>
auto Graph::find_nodes_where(Predicate predicate) -> std::vector<Node> {
    auto filtered_nodes = nodes | std::views::filter(predicate);
    return std::vector<Node>(filtered_nodes.begin(), filtered_nodes.end());
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

auto Database::seed() -> void {
    Seeder seeder;
    auto const graph = new Graph{};
    seeder.seed_graph(*graph);
    this->current_graph = graph;
}

Database::Database(const DatabaseConfig config) : config(config) {
    if (config.from_seed) {
        this->seed();
        return;
    }

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

auto Database::get_graph() const -> const Graph & {
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
        if (words[0] == "INSERT" && words[1] == "NODE" && words[2] == "COMPLEX") {
            auto rest = std::accumulate(words.begin() + 3, words.end(), std::string{},
                                        [](const std::string &a, const std::string &b) {
                                            return a.empty() ? b : a + " " + b;
                                        });
            commands.emplace_back("INSERT NODE COMPLEX", Utils::minifyJson(rest));
            return Query(std::move(commands));
        }
        throw std::invalid_argument("Invalid query");
    }

    // SELECT NODE WHERE id >/</==
    if (words.size() == 5) {
        if (words[0] == "SELECT" && words[1] == "NODE" && words[2] == "WHERE" && words[3] == "id") {
            commands.emplace_back("SELECT NODE WHERE id", words[4]);
        } else {
            throw std::invalid_argument("Invalid query");
        }
    }

    // TODO: Support more complex queries
    // CREATE NODE COMPLEX "name":"[name]", "field1":[data], "field2": [data2] ...
    // CREATE EDGE FROM [node.id] TO [other_node.id], [other_node2.id], ...
    // SELECT WHERE TYPE EQUALS [name] OR/AND TYPE EQUALS [name] WHERE [field_name] ...
    // UPDATE NODE WHERE [some_condition] TO [int/double/bool/string data]

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
}

auto Query::handle_select(Database &db) const -> void {
    logger.debug("SELECT NODE started");
}

auto Query::handle_update_node(Database &db) const -> void {
    logger.debug("UPDATE NODE started");
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
    if (first_command.keyword == "UPDATE NODE") {
        return handle_update_node(db);
    }
    if (first_command.keyword == "SELECT NODE") {
        return handle_select(db);
    }
    throw std::invalid_argument("Unknown command");
}

auto Query::get_commands() const -> const std::vector<Command> & {
    return commands;
}

Logger Database::logger("Database");
Logger Query::logger("Query");
