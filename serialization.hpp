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
        std::ostringstream result;
        if (std::holds_alternative<int>(value)) {
            result << std::get<int>(value);
        } else if (std::holds_alternative<double>(value)) {
            result << std::get<double>(value);
        } else if (std::holds_alternative<bool>(value)) {
            result << (std::get<bool>(value) ? "true" : "false");
        } else if (std::holds_alternative<std::string>(value)) {
            result << "\"" << escape_json(std::get<std::string>(value)) << "\"";
        } else if (std::holds_alternative<std::vector<int> >(value)) {
            const auto &vec = std::get<std::vector<int> >(value);
            result << "[";
            for (size_t i = 0; i < vec.size(); ++i) {
                result << vec[i];
                if (i + 1 < vec.size()) result << ",";
            }
            result << "]";
        } else if (std::holds_alternative<std::vector<std::string> >(value)) {
            const auto &vec = std::get<std::vector<std::string> >(value);
            result << "[";
            for (size_t i = 0; i < vec.size(); ++i) {
                result << "\"" << escape_json(vec[i]) << "\"";
                if (i + 1 < vec.size()) result << ",";
            }
            result << "]";
        } else if (std::holds_alternative<std::vector<bool> >(value)) {
            const auto &vec = std::get<std::vector<bool> >(value);
            result << "[";
            for (size_t i = 0; i < vec.size(); ++i) {
                result << (vec[i] ? "true" : "false");
                if (i + 1 < vec.size()) result << ",";
            }
            result << "]";
        }
        return result.str();
    }

    static std::string serialize_user_defined_value(const UserDefinedValue &udv) {
        std::ostringstream result;
        result << "{";
        for (size_t i = 0; i < udv.values.size(); ++i) {
            const auto &pair = udv.values[i];
            result << "\"" << escape_json(pair.first) << "\":" << serialize_value(pair.second);
            if (i + 1 < udv.values.size()) result << ",";
        }
        result << "}";
        return result.str();
    }

    static std::string serialize_node(const Node &node) {
        std::ostringstream result;
        result << "{";
        result << "\"id\":" << node.id << ",";
        result << "\"type\":\"" << escape_json(node.type) << "\",";
        result << "\"data\":";
        if (std::holds_alternative<BasicValue>(node.data)) {
            result << serialize_value(std::get<BasicValue>(node.data));
        } else if (std::holds_alternative<UserDefinedValue>(node.data)) {
            result << serialize_user_defined_value(std::get<UserDefinedValue>(node.data));
        }
        result << "}";
        return result.str();
    }

    static std::string serialize_edge(const Edge &edge) {
        std::ostringstream result;
        result << "{";
        result << "\"id\":" << edge.id << ",";
        result << "\"from\":" << edge.from << ",";
        result << "\"to\":" << edge.to << ",";
        result << "\"relation\":\"" << escape_json(edge.relation) << "\"";
        result << "}";
        return result.str();
    }

    static std::string serialize_graph(const Graph &graph) {
        std::ostringstream result;
        result << "{";
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
        result << "]";
        result << "}";
        return result.str();
    }
};


#endif //SERIALIZATION_HPP
