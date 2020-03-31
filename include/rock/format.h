#pragma once

#include "types.h"
#include <fmt/format.h>

namespace rock
{

namespace BoardFormatMode
{
    // clang-format off
    constexpr u64 OuterSpaces     = 0b0000000000000001;
    constexpr u64 InnerSpaces     = 0b0000000000000010;
    constexpr u64 OuterBoundaries = 0b0000000000000100;
    constexpr u64 InnerBoundaries = 0b0000000000001000;
    constexpr u64 LabelBottom     = 0b0000000000010000;
    constexpr u64 LabelTop        = 0b0000000000100000;
    constexpr u64 LabelLeft       = 0b0000000001000000;
    constexpr u64 LabelRight      = 0b0000000010000000;
    constexpr u64 UpperCasePieces = 0b0000000100000000;
    // clang-format on

    constexpr u64 Default = OuterSpaces | InnerSpaces | OuterBoundaries | InnerBoundaries |
        LabelLeft | LabelBottom | UpperCasePieces;
    constexpr u64 Compact = 0;
}

struct BoardFormat
{
    u64 mode{BoardFormatMode::Default};
    char empty_char{' '};
};

auto to_string(Player) -> std::string;
auto to_string(Board const&, BoardFormat const& = {}) -> std::string;
auto to_string(BitBoard) -> std::string;
auto to_string(BoardPosition) -> std::string;
auto to_string(Move) -> std::string;

}  // namespace rock

namespace fmt
{

template <>
struct formatter<rock::Player>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(rock::Player p, FormatContext& ctx)
    {
        return format_to(ctx.out(), p == rock::Player::White ? "White" : "Black");
    }
};

template <>
struct formatter<rock::Board>
{
    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin();
        auto end = ctx.end();

        if (it != end)
        {
            switch (*it++)
            {
            case 'c':
                bf.mode = rock::BoardFormatMode::Compact;
                break;
            case 'd':
                bf.mode = rock::BoardFormatMode::Default;
                break;
            case '}':
                break;
            default:
                throw format_error("invalid format");
            }
        }

        if (it != end && *it != '}')
            bf.empty_char = *it;

        if (it != end && *it != '}')
            throw format_error("invalid format");

        return it;
    }

    template <typename FormatContext>
    auto format(rock::Board const& board, FormatContext& ctx)
    {
        return format_to(ctx.out(), "{}", rock::to_string(board, bf));
    }

    rock::BoardFormat bf{};
};

template <>
struct formatter<rock::BitBoard>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(rock::BitBoard const& board, FormatContext& ctx)
    {
        return format_to(ctx.out(), "{}", rock::to_string(board));
    }
};

template <>
struct formatter<rock::BoardPosition>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(rock::BoardPosition pos, FormatContext& ctx)
    {
        return format_to(ctx.out(), "{}{}", static_cast<char>(pos.x() + 'A'), pos.y() + 1);
    }
};

template <>
struct formatter<rock::Move>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(rock::Move m, FormatContext& ctx)
    {
        auto const from = rock::BoardPosition{m.from};
        auto const to = rock::BoardPosition{m.to};
        return format_to(ctx.out(), "{} -> {}", from, to);
    }
};

}  // namespace fmt
