#include "rock/fen.h"

namespace rock
{

auto format_as_fen(rock::Board b) -> std::string
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
            bool const is_white = b[rock::Player::White].at({x, y});
            bool const is_black = b[rock::Player::Black].at({x, y});

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

auto parse_fen_to_board(std::string_view fen) -> std::optional<rock::Board>
{
    auto res = rock::Board{};
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
            res[rock::Player::White].set_bit({x++, y});
        else if (ch == 'p')
            res[rock::Player::Black].set_bit({x++, y});
        else if (ch >= '0' && ch <= '9')
            x += ch - '0';
        else
            return std::nullopt;
    }

    return res;
}

}  // namespace rock
