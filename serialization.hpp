//
// Created by Wiktor Zając on 01/01/2025.
//
#ifndef SERIALIZATION_HPP
#define SERIALIZATION_HPP

#include "database.hpp"
#include <iomanip>
#include <sstream>
#include <string>

class Serialization {
    static Logger logger;

public:
    static std::string escape_json(const std::string &value) {
        logger.debug(std::format("Escaping JSON for value {}", value));

        std::ostringstream escaped;
        for (const auto ch: value) {
            switch (ch) {
                case '"': escaped << "\"";
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
                    // Escape non printable characters
                    if (ch >= 0 && ch <= 31) {
                        escaped << std::format("\\u{:04x}", static_cast<int>(ch));
                    } else {
                        escaped << ch;
                    }
            }
        }

        logger.debug(std::format("Escaped JSON {}", escaped.str()));
        return escaped.str();
    }

    static std::string serialize_value(const BasicValue &value) {
        logger.debug(std::format("Value serialization started for {}", value.toString()));

        const auto data = value.data;
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

    static std::string serialize_user_defined_value(const UserDefinedValue &value) {
        std::ostringstream result;
        result << "{";

        const auto &data = value.get_data();
        for (size_t i = 0; i < data.size(); ++i) {
            const auto &[key, val] = data[i];
            result << "\"" << escape_json(key) << "\":";

            std::visit(
                [&result]<typename T0>(const T0 &v) {
                    using T = std::decay_t<T0>;
                    if constexpr (std::is_same_v<T, BasicValue>) {
                        result << serialize_value(v);
                    } else if constexpr (std::is_same_v<T, UserDefinedValue>) {
                        result << serialize_user_defined_value(v);
                    }
                },
                val);

            if (i + 1 < data.size()) {
                result << ",";
            }
        }

        result << "}";
        return result.str();
    }

    static std::string serialize_node(const Node &node) {
        logger.debug(std::format("Node serialization started for node with id {}", node.id));

        std::ostringstream result;
        result << "{";
        result << "\"id\":" << node.id << ",";
        result << "\"data\":";
        if (std::holds_alternative<BasicValue>(node.data)) {
            result << serialize_value(std::get<BasicValue>(node.data));
        } else if (std::holds_alternative<UserDefinedValue>(node.data)) {
            result << serialize_user_defined_value(std::get<UserDefinedValue>(node.data));
        }
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
        result << R"("relation":")" << escape_json(edge.relation) << "\"";
        result << "}";

        logger.debug(std::format("Edge serialization completed for edge with id {}. Result: {}", edge.id,
                                 result.str()));
        return result.str();
    }

    static std::string serialize_graph(const Graph &graph) {
        logger.debug(std::format("Graph serialization started for graph with name {}", graph.name));

        std::ostringstream result;
        result << "{";
        result << "\"name\":" << "\"" << graph.name << "\",";
        result << "\"nodes\":[";
        for (size_t i = 0; i < graph.nodes.size(); ++i) {
            result << serialize_node(graph.nodes[i]);
            if (i + 1 < graph.nodes.size()) result << ",";
        }
        result << "],";
        result << "\"edges\":[";
        for (size_t i = 0; i < graph.edges.size(); ++i) {
            result << serialize_edge(graph.edges[i]);
            if (i + 1 < graph.edges.size()) result << ",";
        }
        result << "]" << "}";

        logger.info(std::format("Graph serialization completed for graph with name {}", graph.name));
        return result.str();
    }

    static std::string serialize_database(Database &database) {
        std::ostringstream result;

        auto const graphs = database.get_graphs();
        result << "{";
        result << "\"graphs\":[";
        for (size_t i = 0; i < graphs->size(); ++i) {
            result << serialize_graph((*graphs)[i]);
            if (i + 1 < graphs->size()) result << ",";
        }
        result << "]" << "}";

        return result.str();
    }
};

Logger Serialization::logger = Logger("Serialization");

#endif //SERIALIZATION_HPP
