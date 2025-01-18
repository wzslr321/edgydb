//
// Created by Wiktor Zając on 18/01/2025.
//

#ifndef CONDITION_HPP
#define CONDITION_HPP
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

struct Condition {
    std::string field;
    std::string comparator;
    std::string value;
};

struct ConditionGroup {
    std::vector<Condition> conditions;
    std::vector<std::string> operators;
};

ConditionGroup parse_conditions(const std::string &condition_str) {
    ConditionGroup group;
    std::istringstream stream(condition_str);
    std::string token;

    while (stream >> std::quoted(token)) {
        Condition condition;
        condition.field = token;

        stream >> token;
        if (token != "EQ" && token != "NEQ") {
            throw std::invalid_argument("Invalid comparator in condition");
        }
        condition.comparator = token;

        stream >> std::quoted(token);
        condition.value = token;

        group.conditions.push_back(condition);

        if (stream >> token) {
            if (token != "AND" && token != "OR") {
                throw std::invalid_argument("Invalid logical operator");
            }
            group.operators.push_back(token);
        }
    }

    return group;
}

#endif //CONDITION_HPP
