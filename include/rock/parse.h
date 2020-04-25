#pragma once

#include "types.h"
#include <optional>
#include <string_view>

namespace rock
{

/**
 * Constexpr parse functions for parsing string literals in code. If input is
 * not correct, will return something (maybe default-constructed object, maybe
 * partially-filled).
 */
constexpr auto parse_literal_player(std::string_view) -> Player;
constexpr auto parse_literal_board(std::string_view) -> Board;
constexpr auto parse_literal_bit_board(std::string_view) -> BitBoard;

/**
 * Non-constexpr parse functions with simple error handling: either the object
 * can be parsed or not. Not very composable.
 */
auto parse_player(std::string_view) -> std::optional<Player>;
auto parse_board_coordinates(std::string_view) -> std::optional<BoardCoordinates>;
auto parse_move(std::string_view) -> std::optional<Move>;

namespace literals
{
    constexpr auto operator ""_player(char const* str, std::size_t size) -> Player
    {
        return parse_literal_player({str, size});
    }
    constexpr auto operator ""_board(char const* str, std::size_t size) -> Board
    {
        return parse_literal_board({str, size});
    }
    constexpr auto operator ""_bit_board(char const* str, std::size_t size) -> BitBoard
    {
        return parse_literal_bit_board({str, size});
    }
}  // namespace literals

}  // namespace rock

////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementations
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace rock
{

namespace detail
{
    struct LiteralParser
    {
        constexpr explicit LiteralParser(std::string_view str) : str_{str}, it_{str_.begin()} {}

        /**
         * Returns 0 if at end
         */
        constexpr auto get_next_char() -> char
        {
            auto ch = char{};

            do
            {
                if (it_ != str_.end())
                    ch = *(it_++);
            } while (ch == '\n');

            return ch;
        }

    private:
        std::string_view str_{};
        std::string_view::iterator it_{};
    };
}  // namespace detail

constexpr auto parse_literal_player(std::string_view str) -> Player
{
    if (str.empty())
        return Player{};
    else if (str.front() == 'w' || str.front() == 'W' || str == "Player::White")
        return Player::White;
    else
        return Player::Black;
}

constexpr auto parse_literal_board(std::string_view str) -> Board
{
    auto board = Board{};
    auto parser = detail::LiteralParser(str);

    for (auto row = 8; row-- > 0;)
    {
        for (auto col = 0; col < 8; ++col)
        {
            if (auto const ch = parser.get_next_char(); ch == char{})
                return board;
            else if (ch == 'w' || ch == 'W')
                board[Player::White] |= BoardCoordinates{col, row}.bit_board();
            else if (ch == 'b' || ch == 'B')
                board[Player::Black] |= BoardCoordinates{col, row}.bit_board();
        }
    }

    return board;
}

constexpr auto parse_literal_bit_board(std::string_view str) -> BitBoard
{
    auto board = BitBoard{};
    auto parser = detail::LiteralParser(str);

    for (auto row = 8; row-- > 0;)
    {
        for (auto col = 0; col < 8; ++col)
        {
            if (auto const ch = parser.get_next_char(); ch == char{})
                return board;
            else if (ch != ' ')
                board.set_bit({col, row});
        }
    }

    return board;
}

}  // namespace rock
