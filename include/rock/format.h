#pragma once

#include "types.h"
#include <fmt/format.h>

namespace rock
{

auto to_string(Board const&) -> std::string;
auto to_string(BitBoard) -> std::string;
auto to_string(u64) -> std::string;
auto to_string(BoardPosition) -> std::string;
auto to_string(Move) -> std::string;

}  // namespace rock

namespace fmt
{

template <>
struct formatter<rock::Board>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(rock::Board const& board, FormatContext& ctx)
    {
        return format_to(ctx.out(), "{}", rock::to_string(board));
    }
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
