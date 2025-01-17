//
// Created by Wiktor Zając on 17/01/2025.
//

#ifndef UTILS_HPP
#define UTILS_HPP
#include <string>

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
};

#endif //UTILS_HPP
