//
// Created by Wiktor Zając on 27/11/2024.
//

#ifndef GRAPH_HPP
#define GRAPH_HPP
#include <string>
#include <variant>
#include <vector>

using BasicValue = std::variant<int, double, bool, std::string, std::vector<int>, std::vector<std::string>, std::vector<
    bool> >;

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

struct OperationResult {
    std::string message;
};

class Database {
private:
    std::vector<Graph> graphs;

public:
    Database() = default;

    static OperationResult execute_command(const std::string &command);
};

#endif //GRAPH_HPP
