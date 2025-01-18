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

    explicit Comparator(const std::string_view value) {
        if (!is_valid(value)) {
            throw std::invalid_argument(std::format("Invalid comparator:{}", value));
        }
        this->value = value;
    }

    [[nodiscard]]
    static auto is_valid(const std::string_view value) -> bool {
        return std::ranges::contains(valid_values, value);
    }

    [[nodiscard]]
    auto compare(const std::string_view left, const std::string_view right) const -> bool {
        if (value == "EQ") {
            return left == right;
        }
        if (value == "NEQ") {
            return left != right;
        }
        throw std::logic_error(std::format("Unsupported comparator:{}", value));
    }
};

struct LogicalOperator {
    static const inline std::vector<std::string> valid_values{"AND", "OR"};

    std::string value;

    explicit LogicalOperator(const std::string_view value) {
        if (!is_valid(value)) {
            throw std::invalid_argument(std::format("Invalid logical operator: {}", value));
        }
        this->value = value;
    }

    [[nodiscard]]
    static auto is_valid(const std::string_view value) -> bool {
        return std::ranges::contains(valid_values, value);
    }

    [[nodiscard]]
    auto apply(const bool left, const bool right) const -> bool {
        if (value == "AND") {
            return left && right;
        }
        if (value == "OR") {
            return left || right;
        }
        throw std::logic_error(std::format("Unsupported logical operator: {}", value));
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

// TODO: Possibly move to separate Condition.cpp to omit inline specifier
inline auto parse_conditions(const std::string &condition_str) -> ConditionGroup {
    auto group = ConditionGroup{};
    auto stream = std::istringstream(condition_str);
    auto token = std::string{};

    while (stream >> std::quoted(token)) {
        auto condition = Condition{};
        condition.field = token;

        stream >> token;
        condition.comparator = Comparator(token);

        stream >> std::quoted(token);
        condition.value = token;

        group.conditions.push_back(condition);

        if (stream >> token) {
            group.operators.emplace_back(token);
        }
    }

    return group;
}

#endif // CONDITION_HPP
