#include "rock/format.h"
#include <fmt/format.h>
#include <string>

namespace rock
{

auto to_string(Player p) -> std::string
{
    return fmt::format("{}", p);
}

namespace
{
    auto format_pos(Board const& board, int row, int col) -> char
    {
        auto const mask = BoardPosition{col, row}.board();
        bool const p_w = board[Player::White] & mask;
        bool const p_b = board[Player::Black] & mask;
        return p_w ? 'w' : p_b ? 'b' : ' ';
    }
}  // namespace

auto to_string(Board const& board) -> std::string
{
    auto str = std::string{};

    str.append("   +-------------------------------+\n");

    for (auto row = 8; row-- > 0;)
    {
        for (auto col = 0; col < 8; ++col)
        {
            if (col == 0)
            {
                str.append(" ");
                str.push_back('1' + row);
            }
            str.append(" | ");
            str.push_back(format_pos(board, row, col));
        }
        str.append(" | \n");
        if (row != 0)
            str.append("   |---+---+---+---+---+---+---+---|\n");
    }

    str.append("   +-------------------------------+\n");
    str.append("     A   B   C   D   E   F   G   H  ");

    return str;
}

auto to_string(BitBoard board) -> std::string
{
    auto str = std::string{};
    for (auto row = 8; row-- > 0;)
    {
        for (auto col = 0; col < 8; ++col)
            str += board.at({col, row}) ? "x" : "-";
        str += "\n";
    }
    str.pop_back();
    return str;
}

auto to_string(BoardPosition pos) -> std::string
{
    return fmt::format("{}", pos);
}

auto to_string(Move move) -> std::string
{
    return fmt::format("{}", move);
}

}  // namespace rock
