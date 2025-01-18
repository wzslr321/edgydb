//
// Created by Wiktor Zając on 17/01/2025.
//

#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <numeric>

struct Utils {
    static auto minify_json(const std::string &json) -> std::string {
        std::string result;
        bool in_string = false;

        for (const auto current: json) {
            if (current == '"') {
                in_string = !in_string;
            }

            const auto is_not_space = !std::isspace(static_cast<unsigned char>(current));
            if (in_string || is_not_space) {
                result += current;
            }
        }

        return result;
    }

    static auto trim(const std::string &str) -> std::string {
        auto start = str.begin();
        while (start != str.end() && std::isspace(*start)) {
            ++start;
        }

        auto end = str.end();
        do {
            --end;
        } while (std::distance(start, end) > 0 && std::isspace(*end));

        return {start, end + 1};
    }

    // TODO: Check if its usage can be replaced with `trim` if it is okay to trim at the end as well
    static auto trim_leading_spaces(const std::string &str) -> std::string {
        const auto it = std::ranges::find_if(str, [](const unsigned char ch) {
            return !std::isspace(ch);
        });
        return {it, str.end()};
    }

    static auto get_rest_of_space_separated_string(
        const std::vector<std::string> &str,
        const int start) -> std::string {
        return std::accumulate(str.begin() + start, str.end(), std::string{},
                               [](const std::string &a, const std::string &b) {
                                   return a.empty() ? b : a + " " + b;
                               });
    }
};

#endif //UTILS_HPP
