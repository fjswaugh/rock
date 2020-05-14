#pragma once

#include "types.h"
#include <fmt/format.h>

namespace rock
{

namespace BoardFormatMode
{
    // clang-format off
    ROCK_API inline constexpr u64 OuterSpaces     = 0b0000000000000001;
    ROCK_API inline constexpr u64 InnerSpaces     = 0b0000000000000010;
    ROCK_API inline constexpr u64 OuterBoundaries = 0b0000000000000100;
    ROCK_API inline constexpr u64 InnerBoundaries = 0b0000000000001000;
    ROCK_API inline constexpr u64 LabelBottom     = 0b0000000000010000;
    ROCK_API inline constexpr u64 LabelTop        = 0b0000000000100000;
    ROCK_API inline constexpr u64 LabelLeft       = 0b0000000001000000;
    ROCK_API inline constexpr u64 LabelRight      = 0b0000000010000000;
    ROCK_API inline constexpr u64 UpperCasePieces = 0b0000000100000000;
    // clang-format on

    ROCK_API inline constexpr u64 Default = OuterSpaces | InnerSpaces | OuterBoundaries |
        InnerBoundaries | LabelLeft | LabelBottom | UpperCasePieces;
    ROCK_API inline constexpr u64 Compact = 0;
}  // namespace BoardFormatMode

struct ROCK_API BoardFormat
{
    u64 mode{BoardFormatMode::Default};
    char empty_char{' '};
};

ROCK_API auto to_string(Player) -> std::string;
ROCK_API auto to_string(BoardCoordinates) -> std::string;
ROCK_API auto to_string(BitBoard) -> std::string;
ROCK_API auto to_string(Board const&, BoardFormat const& = {}) -> std::string;
ROCK_API auto to_string(Position const&, BoardFormat const& = {}) -> std::string;
ROCK_API auto to_string(Move) -> std::string;

}  // namespace rock

namespace fmt
{

template <>
struct ROCK_API formatter<rock::Player> : formatter<std::string_view>
{
    template <typename FormatContext>
    auto format(rock::Player p, FormatContext& ctx)
    {
        auto const s = p == rock::Player::White ? "White" : "Black";
        return formatter<std::string_view>::format(s, ctx);
    }
};

template <>
struct ROCK_API formatter<rock::GameOutcome> : formatter<std::string_view>
{
    template <typename FormatContext>
    auto format(rock::GameOutcome o, FormatContext& ctx)
    {
        auto const s = [o] {
            switch (o)
            {
            case rock::GameOutcome::Ongoing:
                return "Ongoing";
            case rock::GameOutcome::WhiteWins:
                return "WhiteWins";
            case rock::GameOutcome::BlackWins:
                return "BlackWins";
            case rock::GameOutcome::Draw:
                return "Draw";
            }
            return "";
        }();

        return formatter<std::string_view>::format(s, ctx);
    }
};

template <>
struct ROCK_API formatter<rock::BoardCoordinates>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(rock::BoardCoordinates pos, FormatContext& ctx)
    {
        auto const col = static_cast<char>(pos.x() + 'a');
        auto const row = static_cast<char>(pos.y() + '1');
        return format_to(ctx.out(), "{}{}", col, row);
    }
};

template <>
struct ROCK_API formatter<rock::BitBoard>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(rock::BitBoard const& board, FormatContext& ctx)
    {
        return format_to(ctx.out(), rock::to_string(board));
    }
};

template <>
struct ROCK_API formatter<rock::Board>
{
    auto parse(format_parse_context& ctx) -> format_parse_context::iterator;

    template <typename FormatContext>
    auto format(rock::Board const& board, FormatContext& ctx)
    {
        return format_to(ctx.out(), rock::to_string(board, bf));
    }

    rock::BoardFormat bf{};
};

template <>
struct ROCK_API formatter<rock::Position> : formatter<rock::Board>
{
    template <typename FormatContext>
    auto format(rock::Position const& position, FormatContext& ctx)
    {
        return format_to(ctx.out(), rock::to_string(position, bf));
    }
};

template <>
struct ROCK_API formatter<rock::Move>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(rock::Move m, FormatContext& ctx)
    {
        auto const from = rock::BoardCoordinates{m.from};
        auto const to = rock::BoardCoordinates{m.to};
        return format_to(ctx.out(), "{}-{}", from, to);
    }
};

}  // namespace fmt
