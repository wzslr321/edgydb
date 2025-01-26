//
// Created by Wiktor Zając on 14/01/2025.
//

#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <fmt/core.h>
#include <fmt/color.h>

class Logger {
    std::string name;
    fmt::rgb name_color;
    inline static int log_level = 0;

    static auto generate_color_from_name(const std::string &name) -> fmt::rgb {
        constexpr std::hash<std::string> hasher;
        const size_t hash = hasher(name);

        auto r = static_cast<uint8_t>((hash >> 16) & 0xFF);
        auto g = static_cast<uint8_t>((hash >> 8) & 0xFF);
        auto b = static_cast<uint8_t>(hash & 0xFF);

        constexpr uint8_t min_brightness = 100;
        r = std::max(r, min_brightness);
        g = std::max(g, min_brightness);
        b = std::max(b, min_brightness);

        return fmt::rgb{r, g, b};
    }

    [[nodiscard]]
    auto format_message(const std::string &level, const std::string &message) const -> std::string {
        return fmt::format("[{}] [{}] {}", level, name, message);
    }

    auto print_with_color(const std::string &level, const fmt::rgb &level_color,
                          const std::string &message, std::FILE *pipe = stdout) const -> void {
        fmt::print(pipe, fg(level_color), "[{}] ", level);
        fmt::print(pipe, fg(name_color), "[{}] ", name);
        fmt::print(pipe, "{}\n", message);
    }

public:
    explicit Logger(const std::string &logger_name)
        : name(logger_name), name_color(generate_color_from_name(logger_name)) {
    }

    static auto set_log_level(const int level) -> void {
        log_level = level;
    }

    auto info(const std::string &message) const -> void {
        print_with_color("INFO", fmt::color::light_green, message);
    }

    auto warning(const std::string &message) const -> void {
        print_with_color("WARNING", fmt::color::yellow, message);
    }

    auto error(const std::string &message) const -> void {
        print_with_color("ERROR", fmt::color::red, message, stderr);
    }

    auto debug(const std::string &message) const -> void {
        if (log_level >= 1) {
            print_with_color("DEBUG", fmt::color::gray, message);
        }
    }
};

#endif //LOGGER_HPP
