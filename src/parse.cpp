#include "rock/parse.h"
#include <regex>

// Note, these functions are not efficient in any way

namespace rock
{

auto parse_board_position(std::string_view view) -> std::optional<BoardPosition>
{
    auto str = std::string{view};
    auto const re = std::regex("([a-z]) *([0-9])", std::regex::icase);

    if (auto matches = std::smatch{}; std::regex_search(str, matches, re))
    {
        if (matches.size() != 3)
            return std::nullopt;

        auto const x_str = matches[1].str();
        auto const y_str = matches[2].str();

        if (x_str.size() != 1 || y_str.size() != 1)
            return std::nullopt;

        char const x_char = x_str.front();
        char const y_char = y_str.front();

        return BoardPosition{std::toupper(x_char) - 'A', y_char - '1'};
    }

    return std::nullopt;
}

auto parse_move(std::string_view view) -> std::optional<Move>
{
    auto str = std::string{view};
    auto const re = std::regex("([a-z] *[0-9])[^a-z]*([a-z] [0-9])", std::regex::icase);

    auto matches = std::smatch{};

    if (std::regex_search(str, matches, re))
    {
        if (matches.size() != 3)
            return std::nullopt;

        auto const from_str = matches[1].str();
        auto const to_str = matches[2].str();

        auto const from = parse_board_position(from_str);
        auto const to = parse_board_position(to_str);

        if (from && to)
            return Move{from->data(), to->data()};
    }

    return std::nullopt;
}

}  // namespace rock
