//
// Created by Wiktor Zając on 14/01/2025.
//

#ifndef DESERIALIZATION_HPP
#define DESERIALIZATION_HPP

#include <string>
#include "database.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "Logger.hpp"


class Deserialization {
    static Logger logger;

    static std::string trim(const std::string &str) {
        const auto begin = str.find_first_not_of(" \t\n\r");
        if (begin == std::string::npos) return "";
        const auto end = str.find_last_not_of(" \t\n\r");
        return str.substr(begin, end - begin + 1);
    }

    static std::string parse_string(const std::string &json, size_t &pos) {
        if (json[pos] != '"') throw std::runtime_error("Expected string on pos " + std::to_string(pos));
        ++pos;
        std::string result;
        while (pos < json.size() && json[pos] != '"') {
            if (json[pos] == '\\') {
                ++pos;
                if (pos >= json.size()) throw std::runtime_error("Invalid escape sequence in string");
                switch (json[pos]) {
                    case '"': result += '"';
                        break;
                    case '\\': result += '\\';
                        break;
                    case 'b': result += '\b';
                        break;
                    case 'f': result += '\f';
                        break;
                    case 'n': result += '\n';
                        break;
                    case 'r': result += '\r';
                        break;
                    case 't': result += '\t';
                        break;
                    default: throw std::runtime_error("Invalid escape sequence in string");
                }
            } else {
                result += json[pos];
            }
            ++pos;
        }
        if (pos >= json.size() || json[pos] != '"') throw std::runtime_error("Unterminated string");
        ++pos;
        logger.debug("Successfully parsed string: " + result);
        return result;
    }

    static int parse_int(const std::string &json, size_t &pos) {
        size_t end_pos;
        const int value = std::stoi(json.substr(pos), &end_pos);
        pos += end_pos;
        return value;
    }

    static BasicValue parse_value(const std::string &json, size_t &pos) {
        while (pos < json.size() && isspace(json[pos])) ++pos; // Skip whitespace

        if (json[pos] == '"') {
            return BasicValue(parse_string(json, pos));
        }
        if (isdigit(json[pos]) || json[pos] == '-') {
            return BasicValue(parse_int(json, pos));
        }
        if (json.compare(pos, 4, "true") == 0) {
            pos += 4;
            return BasicValue(true);
        }
        if (json.compare(pos, 5, "false") == 0) {
            pos += 5;
            return BasicValue(false);
        }

        throw std::runtime_error("Invalid value in JSON");
    }

    static Node parse_node(const std::string &json, size_t &pos) {
        if (json[pos] != '{') throw std::runtime_error("Expected object");
        ++pos;

        Node node;
        while (pos < json.size() && json[pos] != '}') {
            std::string key = parse_string(json, pos);
            if (json[pos] != ':') throw std::runtime_error("Expected ':' after key");
            ++pos;

            if (key == "id") {
                node.id = parse_int(json, pos);
            } else if (key == "type") {
                node.type = std::get<std::string>(BasicValue(parse_value(json, pos)).data);
            } else if (key == "data") {
                node.data = parse_value(json, pos);
            }

            if (json[pos] == ',') ++pos;
        }
        if (pos >= json.size() || json[pos] != '}') throw std::runtime_error("Unterminated object");
        ++pos;
        return node;
    }

    static Edge parse_edge(const std::string &json, size_t &pos) {
        if (json[pos] != '{') throw std::runtime_error("Expected object");
        ++pos;

        Edge edge;
        while (pos < json.size() && json[pos] != '}') {
            std::string key = parse_string(json, pos);
            if (json[pos] != ':') throw std::runtime_error("Expected ':' after key");
            ++pos;

            if (key == "id") {
                edge.id = parse_int(json, pos);
            } else if (key == "from") {
                edge.from = parse_int(json, pos);
            } else if (key == "to") {
                edge.to = parse_int(json, pos);
            } else if (key == "relation") {
                edge.relation = std::get<std::string>(BasicValue(parse_value(json, pos)).data);
            }

            if (json[pos] == ',') ++pos;
        }
        if (pos >= json.size() || json[pos] != '}') throw std::runtime_error("Unterminated object");
        ++pos;
        return edge;
    }

    static Graph parse_graph(const std::string &json, size_t &pos) {
        if (json[pos] != '{') throw std::runtime_error("Expected object");
        ++pos;

        Graph graph;
        while (pos < json.size() && json[pos] != '}') {
            std::string key = parse_string(json, pos);
            if (json[pos] != ':') throw std::runtime_error("Expected ':' after key");
            ++pos;

            if (key == "nodes") {
                if (json[pos] != '[') throw std::runtime_error("Expected array");
                ++pos;
                while (pos < json.size() && json[pos] != ']') {
                    graph.nodes.push_back(parse_node(json, pos));
                    if (json[pos] == ',') ++pos; // Skip comma
                }
                if (pos >= json.size() || json[pos] != ']') throw std::runtime_error("Unterminated array");
                ++pos;
            } else if (key == "edges") {
                if (json[pos] != '[') throw std::runtime_error("Expected array");
                ++pos;
                while (pos < json.size() && json[pos] != ']') {
                    graph.edges.push_back(parse_edge(json, pos));
                    if (json[pos] == ',') ++pos;
                }
                if (pos >= json.size() || json[pos] != ']') throw std::runtime_error("Unterminated array");
                ++pos;
            } else if (key == "name") {
                graph.name = parse_string(json, pos);
                ++pos;
            }

            if (json[pos] == ',') ++pos;
        }
        if (pos >= json.size() || json[pos] != '}') throw std::runtime_error("Unterminated object");
        ++pos;
        return graph;
    }

public:
    static std::vector<Graph> deserialize_graphs(const std::string &json) {
        size_t pos = 0;

        if (json[pos] != '{') throw std::runtime_error("Expected object");
        ++pos;

        std::vector<Graph> graphs;
        while (pos < json.size() && json[pos] != '}') {
            if (const std::string key = parse_string(json, pos); key == "graphs") {
                if (json[pos] != ':') throw std::runtime_error("Expected ':' after key");
                ++pos;

                if (json[pos] != '[') throw std::runtime_error("Expected array");
                ++pos;

                while (pos < json.size() && json[pos] != ']') {
                    graphs.push_back(parse_graph(json, pos));
                    if (json[pos] == ',') ++pos;
                }
                if (pos >= json.size() || json[pos] != ']') throw std::runtime_error("Unterminated array");
                ++pos;
            }

            if (json[pos] == ',') ++pos;
        }

        if (pos >= json.size() || json[pos] != '}') throw std::runtime_error("Unterminated object");
        ++pos;

        return graphs;
    }
};

Logger Deserialization::logger("Deserialization");

#endif //DESERIALIZATION_HPP

