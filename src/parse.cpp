#include "rock/parse.h"
#include <algorithm>
#include <regex>

// Note, these functions are not efficient in any way

namespace rock
{

auto parse_player(std::string_view orig) -> std::optional<Player>
{
    auto lowercase = std::string{orig};
    for (char& ch : lowercase)
        ch = static_cast<char>(std::tolower(ch));

    if (lowercase == "w" || lowercase == "white" || orig == "Player::White")
        return Player::White;
    if (lowercase == "b" || lowercase == "black" || orig == "Player::Black")
        return Player::Black;
    return std::nullopt;
}

auto parse_board_position(std::string_view view) -> std::optional<BoardCoordinates>
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

        return BoardCoordinates{std::toupper(x_char) - 'A', y_char - '1'};
    }

    return std::nullopt;
}

auto parse_move(std::string_view view) -> std::optional<Move>
{
    auto str = std::string{view};
    auto const re = std::regex("([a-z] *[0-9])[^a-z]*([a-z] *[0-9])", std::regex::icase);

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
            return Move{*from, *to};
    }

    return std::nullopt;
}

}  // namespace rock
