//
// Created by Wiktor Zając on 14/01/2025.
//

#ifndef DESERIALIZATION_HPP
#define DESERIALIZATION_HPP

#include <string>
#include <stdexcept>

#include "Logger.hpp"

struct Deserialization {
    // TODO: do that eveywehre else
    inline static auto logger = Logger("Deserialization");

    static std::string parse_string(const std::string &json, size_t &pos) {
        logger.debug(std::format("Deserialization of string started at pos {}", pos));
        if (json[pos] != '"') throw std::runtime_error(std::format("Expected string on pos {}", pos));

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
        logger.debug(std::format("Deserialization for int started at pos {}", pos));

        size_t end_pos;
        const int value = std::stoi(json.substr(pos), &end_pos);
        pos += end_pos;

        logger.debug(std::format("Deserialization for int finished with {}", value));
        return value;
    }

    static BasicValue parse_value(const std::string &json, size_t &pos) {
        logger.debug(std::format("Deserialization for BasicValue started at pos {}", pos));
        while (pos < json.size() && isspace(json[pos])) ++pos;

        if (json[pos] == '"') {
            return BasicValue(parse_string(json, pos));
        }
        if (isdigit(json[pos]) || json[pos] == '-') {
            return BasicValue(parse_int(json, pos));
        }
        if (json.compare(pos, 4, "true") == 0) {
            pos += 4;
            logger.debug(std::format("Deserialization for BasicValue finished with boolean true"));
            return BasicValue(true);
        }
        if (json.compare(pos, 5, "false") == 0) {
            pos += 5;
            logger.debug(std::format("Deserialization for BasicValue finished with boolean false"));
            return BasicValue(false);
        }

        throw std::runtime_error("Invalid value in JSON");
    }

    static UserDefinedValue parse_user_defined_value(const std::string &json, size_t &pos) {
        logger.debug(std::format("Deserialization for UserDefinedValue started at pos {}", pos));

        if (json[pos] != '{') throw std::runtime_error("Expected object for UserDefinedValue");
        ++pos;

        std::vector<std::pair<std::string, std::variant<BasicValue, UserDefinedValue> > > data;
        while (pos < json.size() && json[pos] != '}') {
            std::string key = parse_string(json, pos);
            if (json[pos] != ':') throw std::runtime_error("Expected ':' in UserDefinedValue");
            ++pos;

            // TODO: possibly dont care about nested
            if (json[pos] == '{') {
                UserDefinedValue value = parse_user_defined_value(json, pos);
                data.emplace_back(key, value);
            } else {
                BasicValue value = parse_value(json, pos);
                data.emplace_back(key, value);
            }

            if (json[pos] == ',') ++pos;
        }

        if (pos >= json.size() || json[pos] != '}')
            throw std::runtime_error("Unterminated UserDefinedValue object");
        ++pos;

        logger.debug("Deserialization finished for UserDefinedValue");
        return UserDefinedValue(data);
    }

    static Node parse_node(const std::string &json, size_t &pos) {
        logger.debug(std::format("Deserialization for Node started at pos {}", pos));
        if (json[pos] != '{') throw std::runtime_error("Expected object");
        ++pos;

        Node node;
        while (pos < json.size() && json[pos] != '}') {
            std::string key = parse_string(json, pos);
            if (json[pos] != ':') throw std::runtime_error("Expected ':' after key");
            ++pos;

            if (key == "id") {
                node.id = parse_int(json, pos);
            } else if (key == "data") {
                if (json[pos] == '{') {
                    node.data = parse_user_defined_value(json, pos);
                } else {
                    node.data = parse_value(json, pos);
                }
            }

            if (json[pos] == ',') ++pos;
        }
        if (pos >= json.size() || json[pos] != '}') throw std::runtime_error("Unterminated object");
        ++pos;
        logger.debug(std::format("Deserialization finished for node with id {}", node.id));
        return node;
    }

    static Edge parse_edge(const std::string &json, size_t &pos) {
        logger.debug(std::format("Deserialization for Edge started at pos {}", pos));
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
        logger.debug(std::format("Deserialization for Edge with id {} finished", edge.id));
        return edge;
    }

    static Graph parse_graph(const std::string &json, size_t &pos) {
        logger.info(std::format("Parsing of graph started at pos {}", pos));
        if (json[pos] != '{') throw std::runtime_error("Expected object");
        ++pos;

        Graph graph;
        while (pos < json.size() && json[pos] != '}') {
            std::string key = parse_string(json, pos);
            if (json[pos] != ':') throw std::runtime_error("Expected ':' after key");
            ++pos;

            if (key == "name") {
                graph.name = parse_string(json, pos);
                ++pos;
            } else if (key == "nodes") {
                if (json[pos] != '[') throw std::runtime_error("Expected array");
                ++pos;
                while (pos < json.size() && json[pos] != ']') {
                    graph.nodes.push_back(parse_node(json, pos));
                    if (json[pos] == ',') ++pos;
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
            }

            if (json[pos] == ',') ++pos;
        }
        if (pos >= json.size() || json[pos] != '}') throw std::runtime_error("Unterminated object");
        ++pos;
        logger.info(std::format("Parsing finished for graph with name {} containing {} nodes and {} edges", graph.name,
                                graph.nodes.size(), graph.edges.size()));
        return std::move(graph);
    }

    static std::vector<Graph> parse_graphs(const std::string &json) {
        size_t pos = 0;
        logger.info("Parsing started for graphs");

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
                    graphs.emplace_back(parse_graph(json, pos));
                    if (json[pos] == ',') ++pos;
                }
                if (pos >= json.size() || json[pos] != ']') throw std::runtime_error("Unterminated array");
                ++pos;
            }

            if (json[pos] == ',') ++pos;
        }

        if (pos >= json.size() || json[pos] != '}') throw std::runtime_error("Unterminated object");
        ++pos;

        logger.info(std::format("Parsing finished for {} graphs in total", graphs.size()));
        return std::move(graphs);
    }
};

#endif //DESERIALIZATION_HPP

