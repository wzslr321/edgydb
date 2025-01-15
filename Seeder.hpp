//
// Created by Wiktor Zając on 15/01/2025.
//
#pragma once

#ifndef SEEDER_HPP
#define SEEDER_HPP

#include "database.hpp"
#include <string>
#include <vector>
#include <utility>

class Seeder {
    int node_counter = 0;
    int edge_counter = 0;

    std::vector<std::string> dummy_node_types = {"Person", "Company", "Location"};
    std::vector<std::string> dummy_person_names = {"Remi", "Bob", "Charlie", "Diana"};
    std::vector<std::string> dummy_companies = {"EvilCorp", "InnovateX", "DataSolutions"};
    std::vector<std::string> dummy_locations = {"New York", "London", "Tokyo"};
    std::vector<std::string> dummy_graph_names = {"Fishermen", "Policemen", "Firefighters"};

    std::vector<std::string> dummy_relations = {"Knows", "WorksFor", "LocatedIn"};


    auto create_user_defined_node(const UserDefinedValue &data) -> Node {
        return Node{++node_counter, data};
    }

    auto create_edge(const int from, const int to, const std::string &relation) -> Edge {
        return Edge{++edge_counter, from, to, relation};
    }

    static auto create_user_defined_value() -> UserDefinedValue {
        return UserDefinedValue{
            {
                {"name", BasicValue("SampleObject")},
                {"active", BasicValue(true)},
                {"priority", BasicValue(42)},
                {
                    "info", UserDefinedValue{
                        {
                            {"name", BasicValue("SampleObjectInfo")},
                            {"age", BasicValue(15)},
                            {"mother_name", BasicValue("Karen")},
                        },
                    },
                }
            }
        };
    }

public:
    void seed_graph(Graph &graph, const int num_nodes = 10, const int num_edges = 5) {
        graph.name = dummy_graph_names[(num_nodes + num_edges) % 2];

        for (int i = 0; i < num_nodes; ++i) {
            if (i % 3 == 0) {
                graph.nodes.push_back(Node{
                    ++node_counter,
                    BasicValue(dummy_person_names[i % dummy_person_names.size()])
                });
            } else if (i % 3 == 1) {
                graph.nodes.push_back(Node{
                    ++node_counter,
                    BasicValue(dummy_companies[i % dummy_companies.size()])
                });
            } else {
                graph.nodes.push_back(create_user_defined_node(
                    create_user_defined_value()));
            }
        }
        for (int i = 0; i < num_edges; ++i) {
            const int from = (i + 1) % num_nodes;
            const int to = (i + 2) % num_nodes;
            graph.edges.push_back(create_edge(from, to, dummy_relations[i % dummy_relations.size()]));
        }
    }
};

#endif // SEEDER_HPP
