#include "rock/format.h"
#include <fmt/format.h>
#include <string>

auto fmt::formatter<rock::Board>::parse(format_parse_context& ctx) -> format_parse_context::iterator
{
    auto it = ctx.begin();
    auto end = ctx.end();

    // If there is a format string, start the mode from nothing
    if (it != end && *it != '}')
        bf.mode = {};

    while (it != end && *it != '}')
    {
        switch (*it++)
        {
        case 's':
            bf.mode ^= rock::BoardFormatMode::InnerSpaces;
            break;
        case 'S':
            bf.mode ^= rock::BoardFormatMode::OuterSpaces;
            break;
        case 'b':
            bf.mode ^= rock::BoardFormatMode::InnerBoundaries;
            break;
        case 'B':
            bf.mode ^= rock::BoardFormatMode::OuterBoundaries;
            break;
        case 'u':
            bf.mode ^= rock::BoardFormatMode::UpperCasePieces;
            break;
        case '^':
            bf.mode ^= rock::BoardFormatMode::LabelTop;
            break;
        case '<':
            bf.mode ^= rock::BoardFormatMode::LabelLeft;
            break;
        case '>':
            bf.mode ^= rock::BoardFormatMode::LabelRight;
            break;
        case 'v':
            bf.mode ^= rock::BoardFormatMode::LabelBottom;
            break;
        case 'd':
            bf.mode |= rock::BoardFormatMode::Default;
            break;
        case 'a':
            bf.mode |= ~rock::u64{};
            break;
        case 'e':
            if (it == end)
                throw format_error("invalid format");
            bf.empty_char = *it++;
            break;
        default:
            throw format_error("invalid format");
        }
    }

    if (it != end && *it != '}')
        throw format_error("invalid format");

    return it;
}

namespace rock
{

auto to_string(Player p) -> std::string
{
    return fmt::format("{}", p);
}

auto to_string(BoardCoordinates pos) -> std::string
{
    return fmt::format("{}", pos);
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

namespace
{
    auto format_pos(Board const& board, BoardCoordinates pos, BoardFormat const& bf) -> char
    {
        bool const p_w = board[Player::White].at(pos);
        bool const p_b = board[Player::Black].at(pos);

        char const white_char = (bf.mode & BoardFormatMode::UpperCasePieces) ? 'W' : 'w';
        char const black_char = (bf.mode & BoardFormatMode::UpperCasePieces) ? 'B' : 'b';

        return p_w ? white_char : p_b ? black_char : bf.empty_char;
    }

    auto make_outer_left(int row, BoardFormat const& bf) -> std::string
    {
        auto str = std::string{};

        if (bf.mode & BoardFormatMode::OuterSpaces)
            str.push_back(' ');

        if (bf.mode & BoardFormatMode::LabelLeft)
        {
            str.push_back('1' + row);
            if (bf.mode & BoardFormatMode::OuterSpaces)
                str.push_back(' ');
        }

        if (bf.mode & BoardFormatMode::OuterBoundaries)
            str.push_back('|');

        return str;
    }

    auto make_inner_row(int row, Board const& board, BoardFormat const& bf) -> std::string
    {
        auto str = std::string{};

        if (bf.mode & BoardFormatMode::InnerSpaces)
            str.push_back(' ');

        for (auto col = 0; col < 8; ++col)
        {
            str.push_back(format_pos(board, {col, row}, bf));

            if (bf.mode & BoardFormatMode::InnerSpaces)
                str.push_back(' ');

            if (col < 7 && (bf.mode & BoardFormatMode::InnerBoundaries))
            {
                str.push_back('|');
                if (bf.mode & BoardFormatMode::InnerSpaces)
                    str.push_back(' ');
            }
        }

        return str;
    }

    auto make_outer_right(int row, BoardFormat const& bf) -> std::string
    {
        auto str = std::string{};

        if (bf.mode & BoardFormatMode::OuterBoundaries)
            str.push_back('|');

        if (bf.mode & BoardFormatMode::OuterSpaces)
            str.push_back(' ');

        if (bf.mode & BoardFormatMode::LabelRight)
        {
            str.push_back('1' + row);
            if (bf.mode & BoardFormatMode::OuterSpaces)
                str.push_back(' ');
        }

        return str;
    }

    auto make_row(int row, Board board, BoardFormat const& bf) -> std::string
    {
        return make_outer_left(row, bf) + make_inner_row(row, board, bf) +
            make_outer_right(row, bf);
    }

    auto make_horizontal_label(BoardFormat const& bf) -> std::string
    {
        auto left = make_outer_left(0, bf);
        auto right = make_outer_right(0, bf);

        std::fill(left.begin(), left.end(), ' ');
        std::fill(right.begin(), right.end(), ' ');

        auto centre = std::string{};

        if (bf.mode & BoardFormatMode::InnerSpaces)
            centre.push_back(' ');

        for (auto col = 0; col < 8; ++col)
        {
            centre.push_back('A' + col);

            if (bf.mode & BoardFormatMode::InnerSpaces)
                centre.push_back(' ');

            if (col < 7 && (bf.mode & BoardFormatMode::InnerBoundaries))
            {
                centre.push_back(' ');
                if (bf.mode & BoardFormatMode::InnerSpaces)
                    centre.push_back(' ');
            }
        }

        return left + centre + right;
    }

    auto make_horizontal_outer_boundary(BoardFormat const& bf) -> std::string
    {
        assert(bf.mode & BoardFormatMode::OuterBoundaries);

        auto left = make_outer_left(0, bf);
        auto right = make_outer_right(0, bf);
        auto centre = make_inner_row(0, Board{}, bf);

        std::fill(left.begin(), left.end(), ' ');
        std::fill(right.begin(), right.end(), ' ');
        std::fill(centre.begin(), centre.end(), '-');

        left.back() = '+';
        right.front() = '+';

        return left + centre + right;
    }

    auto make_horizontal_inner_boundary(BoardFormat const& bf) -> std::string
    {
        assert(bf.mode & BoardFormatMode::InnerBoundaries);

        auto left = make_outer_left(0, bf);
        auto right = make_outer_right(0, bf);

        std::fill(left.begin(), left.end(), ' ');
        std::fill(right.begin(), right.end(), ' ');

        if (bf.mode & BoardFormatMode::OuterBoundaries)
        {
            left.back() = '|';
            right.front() = '|';
        }

        auto centre = std::string{};

        if (bf.mode & BoardFormatMode::InnerSpaces)
            centre.push_back('-');

        for (auto col = 0; col < 8; ++col)
        {
            centre.push_back('-');

            if (bf.mode & BoardFormatMode::InnerSpaces)
                centre.push_back('-');

            if (col < 7)
            {
                centre.push_back('+');
                if (bf.mode & BoardFormatMode::InnerSpaces)
                    centre.push_back('-');
            }
        }

        return left + centre + right;
    }
}  // namespace

auto to_string(Board const& board, BoardFormat const& bf) -> std::string
{
    auto str = std::string{};

    if (bf.mode & BoardFormatMode::LabelTop)
        str += make_horizontal_label(bf) + "\n";

    if (bf.mode & BoardFormatMode::OuterBoundaries)
        str += make_horizontal_outer_boundary(bf) + "\n";

    for (auto row = 8; row-- > 0;)
    {
        str += make_row(row, board, bf) + "\n";

        if ((bf.mode & BoardFormatMode::InnerBoundaries) && row > 0)
            str += make_horizontal_inner_boundary(bf) + "\n";
    }

    if (bf.mode & BoardFormatMode::OuterBoundaries)
        str += make_horizontal_outer_boundary(bf) + "\n";

    if (bf.mode & BoardFormatMode::LabelBottom)
        str += make_horizontal_label(bf) + "\n";

    // Don't include the final new line
    str.pop_back();

    return str;
}

auto to_string(Position const& position, BoardFormat const& bf) -> std::string
{
    return fmt::format(
        "Player to move: {}\n{}", position.player_to_move(), to_string(position.board(), bf));
}

auto to_string(Move move) -> std::string
{
    return fmt::format("{}", move);
}

}  // namespace rock
