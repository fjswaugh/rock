#pragma once

#include "types.h"
#include <optional>
#include <string_view>

namespace rock
{

constexpr auto parse_literal_board(std::string_view) -> Board;
constexpr auto parse_literal_bit_board(std::string_view) -> BitBoard;

namespace literals
{
    constexpr auto operator ""_board(char const* str, std::size_t size) -> Board
    {
        return parse_literal_board({str, size});
    }
}  // namespace literals

/**
 * Extremely simple parse functions, either the object can be parsed or not,
 * non-composable
 */
auto parse_board_position(std::string_view) -> std::optional<BoardPosition>;
auto parse_move(std::string_view) -> std::optional<Move>;

}  // namespace rock

////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementations
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace rock
{

constexpr auto parse_literal_board(std::string_view str) -> Board
{
    auto board = Board{};

    auto it = str.begin();
    for (auto row = 8; row-->0;)
    {
        for (auto col=0; col<8;++col)
        {
            if (it == str.end())
                return board;

            char const ch = *(it++);

            if (ch == 'w' || ch == 'W')
                board.pieces[bool(Player::White)] |= BoardPosition{col, row}.board();
            if (ch == 'b' || ch == 'B')
                board.pieces[bool(Player::Black)] |= BoardPosition{col, row}.board();
        }
        if (it != str.end() && *it == '\n')
            ++it;
    }

    return board;
}

constexpr auto parse_literal_bit_board(std::string_view str) -> BitBoard
{
    auto board = BitBoard{};

    auto it = str.begin();
    for (auto row = 8; row-->0;)
    {
        for (auto col=0; col<8;++col)
        {
            if (it == str.end())
                return board;

            if (char const ch = *(it++); ch != ' ')
                board.set_bit({col, row});
        }
        if (it != str.end() && *it == '\n')
            ++it;
    }

    return board;
}

}  // namespace rock
