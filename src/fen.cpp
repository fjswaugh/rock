#include "rock/fen.h"
#include <algorithm>

namespace rock
{

auto format_as_fen(Board const& b) -> std::string
{
    auto res = std::string{};

    auto const flush = [&](int& n) {
        if (n > 0)
            res.push_back('0' + static_cast<char>(n));
        n = 0;
    };

    for (auto y = 8; y-- > 0;)
    {
        auto n = 0;

        for (auto x = 0; x < 8; ++x)
        {
            bool const is_white = b[Player::White].at({x, y});
            bool const is_black = b[Player::Black].at({x, y});

            if (is_white || is_black)
                flush(n);

            if (is_white)
                res.push_back('P');
            else if (is_black)
                res.push_back('p');
            else
                ++n;
        }
        flush(n);

        if (y > 0)
            res.push_back('/');
    }

    return res;
}

auto format_as_fen(Position const& p) -> std::string
{
    auto str = format_as_fen(p.board());
    str.push_back(' ');
    str.push_back(p.player_to_move() == Player::White ? 'w' : 'b');
    return str;
}

auto parse_fen_to_board(std::string_view fen) -> std::optional<Board>
{
    auto res = Board{};
    auto x = int{};
    auto y = int{7};

    for (char const ch : fen)
    {
        if (ch == '/')
        {
            x = 0;
            --y;
            continue;
        }

        if (x < 0 || x >= 8 || y < 0 || y >= 8)
            return std::nullopt;

        if (ch == 'P')
            res[Player::White].set_bit({x++, y});
        else if (ch == 'p')
            res[Player::Black].set_bit({x++, y});
        else if (ch >= '0' && ch <= '9')
            x += ch - '0';
        else
            return std::nullopt;
    }

    return res;
}

auto parse_fen_to_position(std::string_view fen_string) -> std::optional<Position>
{
    auto board = parse_fen_to_board(fen_string);
    if (!board)
        return std::nullopt;

    auto it = std::find(fen_string.begin(), fen_string.end(), ' ');
    if (it == fen_string.end() || ++it == fen_string.end())
        return std::nullopt;

    if (*it == 'w')
        return Position{*board, Player::White};
    else if (*it == 'b')
        return Position{*board, Player::Black};
    else
        return std::nullopt;
}

}  // namespace rock
