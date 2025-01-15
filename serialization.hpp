//
// Created by Wiktor Zając on 01/01/2025.
//
#pragma once

#include "database.hpp"
#ifndef SERIALIZATION_HPP
#define SERIALIZATION_HPP
#include <iomanip>
#include <sstream>
#include <string>

class Serialization {
    static Logger logger;

public:
    static std::string escape_json(const std::string &value) {
        std::ostringstream escaped;
        for (const auto &ch: value) {
            switch (ch) {
                case '"': escaped << "\\\"";
                    break;
                case '\\': escaped << "\\\\";
                    break;
                case '\b': escaped << "\\b";
                    break;
                case '\f': escaped << "\\f";
                    break;
                case '\n': escaped << "\\n";
                    break;
                case '\r': escaped << "\\r";
                    break;
                case '\t': escaped << "\\t";
                    break;
                default:
                    if (ch >= 0 && ch <= 31) {
                        escaped << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(ch);
                    } else {
                        escaped << ch;
                    }
            }
        }
        return escaped.str();
    }

    static std::string serialize_value(const BasicValue &value) {
        logger.debug(std::format("Value serialization started for {}", value.toString()));

        auto data = value.data;
        std::ostringstream result;
        if (std::holds_alternative<int>(data)) {
            result << std::get<int>(data);
        } else if (std::holds_alternative<double>(data)) {
            result << std::get<double>(data);
        } else if (std::holds_alternative<bool>(data)) {
            result << (std::get<bool>(data) ? "true" : "false");
        } else if (std::holds_alternative<std::string>(data)) {
            result << "\"" << escape_json(std::get<std::string>(data)) << "\"";
        }

        logger.debug(std::format("Value serialization for {} finished. Result: {}", value.toString(),
                                 result.str())
        );
        return result.str();
    }

    static std::string serialize_node(const Node &node) {
        logger.debug(std::format("Node serialization started for node with id {}", node.id));

        std::ostringstream result;
        result << "{";
        result << "\"id\":" << node.id << ",";
        result << "\"type\":\"" << escape_json(node.type) << "\",";
        result << "\"data\":";
        result << serialize_value(node.data);
        result << "}";

        logger.debug(std::format("Node serialization completed for node with id {}. Result: {}", node.id,
                                 result.str()));
        return result.str();
    }

    static std::string serialize_edge(const Edge &edge) {
        logger.debug(std::format("Edge serialization started for edge with id {}", edge.id));

        std::ostringstream result;
        result << "{";
        result << "\"id\":" << edge.id << ",";
        result << "\"from\":" << edge.from << ",";
        result << "\"to\":" << edge.to << ",";
        result << "\"relation\":\"" << escape_json(edge.relation) << "\"";
        result << "}";

        logger.debug(std::format("Edge serialization completed for edge with id {}. Result: {}", edge.id,
                                 result.str()));
        return result.str();
    }

    static std::string serialize_graph(const std::unique_ptr<Graph> &graph) {
        logger.debug(std::format("Graph serialization started for graph with name {}", graph->name));

        std::ostringstream result;
        result << "{";
        result << "\"name\":" << "\"" << graph->name << "\",";
        result << "\"nodes\":[";
        for (size_t i = 0; i < graph->nodes.size(); ++i) {
            result << serialize_node(graph->nodes[i]);
            if (i + 1 < graph->nodes.size()) result << ",";
        }
        result << "],";
        result << "\"edges\":[";
        for (size_t i = 0; i < graph->edges.size(); ++i) {
            result << serialize_edge(graph->edges[i]);
            if (i + 1 < graph->edges.size()) result << ",";
        }
        result << "]";
        result << "}";

        logger.info(std::format("Graph serialization completed for graph with name {}", graph->name));
        return result.str();
    }

    static std::string serialize_database(Database &database) {
        std::ostringstream result;

        auto graphs = database.get_graphs();
        result << "{";
        result << "\"graphs\":[";
        for (size_t i = 0; i < graphs.size(); ++i) {
            result << Serialization::serialize_graph(graphs[i]);
            if (i + 1 < graphs.size()) result << ",";
        }
        result << "]";
        result << "}";

        return result.str();
    }
};

Logger Serialization::logger = Logger("Serialization");

#endif //SERIALIZATION_HPP
