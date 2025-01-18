//
// Created by Wiktor Zając on 18/01/2025.
//

#ifndef CONDITION_HPP
#define CONDITION_HPP

#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>

struct Comparator {
    static const inline std::vector<std::string> valid_values{"EQ", "NEQ"};

    std::string value;

    explicit Comparator(const std::string &value) {
        if (!is_valid(value)) {
            throw std::invalid_argument("Invalid comparator: " + value);
        }
        this->value = value;
    }

    static bool is_valid(const std::string &value) {
        return std::ranges::find(valid_values, value) != valid_values.end();
    }

    bool compare(const std::string &left, const std::string &right) const {
        if (value == "EQ") {
            return left == right;
        }
        if (value == "NEQ") {
            return left != right;
        }
        throw std::logic_error("Unsupported comparator: " + value);
    }
};

struct LogicalOperator {
    static const inline std::vector<std::string> valid_values{"AND", "OR"};

    std::string value;

    explicit LogicalOperator(const std::string &value) {
        if (!is_valid(value)) {
            throw std::invalid_argument("Invalid logical operator: " + value);
        }
        this->value = value;
    }

    static bool is_valid(const std::string &value) {
        return std::ranges::find(valid_values, value) != valid_values.end();
    }

    bool apply(const bool left, const bool right) const {
        if (value == "AND") {
            return left && right;
        }
        if (value == "OR") {
            return left || right;
        }
        throw std::logic_error("Unsupported logical operator: " + value);
    }
};

struct Condition {
    std::string field;
    std::string value;
    Comparator comparator;

    Condition() : comparator("EQ") {
    }

    Condition(std::string field, std::string value, Comparator comparator)
        : field(std::move(field)), value(std::move(value)), comparator(std::move(comparator)) {
    }
};

struct ConditionGroup {
    std::vector<Condition> conditions;
    std::vector<LogicalOperator> operators;
};

inline ConditionGroup parse_conditions(const std::string &condition_str) {
    ConditionGroup group;
    std::istringstream stream(condition_str);
    std::string token;

    while (stream >> std::quoted(token)) {
        Condition condition;
        condition.field = token;

        stream >> token;
        condition.comparator = Comparator(token);

        stream >> std::quoted(token);
        condition.value = token;

        group.conditions.push_back(condition);

        if (stream >> token) {
            group.operators.emplace_back(LogicalOperator(token));
        }
    }

    return group;
}

#endif // CONDITION_HPP
