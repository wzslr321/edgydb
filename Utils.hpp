//
// Created by Wiktor Zając on 17/01/2025.
//

#ifndef UTILS_HPP
#define UTILS_HPP
#include <string>
#include <numeric>

struct Utils {
    static std::string minifyJson(const std::string &json) {
        std::string result;
        bool inString = false;

        for (size_t i = 0; i < json.size(); ++i) {
            char current = json[i];

            if (current == '"') {
                inString = !inString;
            }

            if (inString) {
                result += current;
            } else {
                if (!std::isspace(static_cast<unsigned char>(current))) {
                    result += current;
                }
            }
        }

        return result;
    }

    static std::string trim_leading_spaces(const std::string &str) {
        const auto it = std::ranges::find_if(str, [](const unsigned char ch) {
            return !std::isspace(ch);
        });
        return std::string(it, str.end());
    }

    static std::string get_rest_of_space_separated_string(const std::vector<std::string> &str, const int start) {
        return std::accumulate(str.begin() + start, str.end(), std::string{},
                               [](const std::string &a, const std::string &b) {
                                   return a.empty() ? b : a + " " + b;
                               });
    }
};

#endif //UTILS_HPP
