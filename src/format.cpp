#include "rock/format.h"
#include <fmt/format.h>
#include <string>

namespace rock
{

namespace
{
    auto format_pos(Board const& board, int row, int col) -> char
    {
        auto const mask = BoardPosition{col, row}.board();
        bool const p0 = board.pieces[0] & mask;
        bool const p1 = board.pieces[1] & mask;
        return p0 ? 'w' : p1 ? 'b' : ' ';
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

auto to_string(u64 board) -> std::string
{
    return to_string(BitBoard{board});
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
