#pragma once

#include "rock/types.h"

namespace rock
{

struct CirclesContainer
{
    u64 data[64][8];
};

struct DirectionsContainer
{
    u64 data[64][4];
};

constexpr auto make_all_directions() -> DirectionsContainer;
constexpr auto make_all_circles() -> CirclesContainer;

}  // namespace rock

////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementations
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace rock
{

namespace detail
{
    constexpr auto my_abs(int x) -> int { return x < 0 ? -x : x; }

    constexpr auto my_max(int x1, int x2) -> int { return x1 > x2 ? x1 : x2; }

    constexpr auto make_circle(BoardCoordinates centre, int radius) -> BitBoard
    {
        auto board = BitBoard{};

        for (auto i = 0; i < 64; ++i)
        {
            auto const pos = BoardCoordinates{i};
            if (my_max(my_abs(pos.x() - centre.x()), my_abs(pos.y() - centre.y())) <= radius)
                board.set_bit(pos);
        }

        return board;
    }

    constexpr auto make_horizontal(BoardCoordinates pos) -> BitBoard
    {
        auto res = BitBoard{};
        for (auto x = 0; x < 8; ++x)
            res.set_bit({x, pos.y()});
        return res;
    }

    constexpr auto make_vertical(BoardCoordinates pos) -> BitBoard
    {
        auto res = BitBoard{};
        for (auto y = 0; y < 8; ++y)
            res.set_bit({pos.x(), y});
        return res;
    }

    constexpr auto make_positive_diagonal(BoardCoordinates pos) -> BitBoard
    {
        auto res = BitBoard{};
        for (auto x = 0; x < 8; ++x)
            if (auto const y = pos.y() + x - pos.x(); y >= 0 && y < 8)
                res.set_bit({x, y});
        return res;
    }

    constexpr auto make_negative_diagonal(BoardCoordinates pos) -> BitBoard
    {
        auto res = BitBoard{};
        for (auto x = 0; x < 8; ++x)
            if (auto const y = pos.y() + pos.x() - x; y >= 0 && y < 8)
                res.set_bit({x, y});
        return res;
    }
}  // namespace detail

constexpr auto make_all_directions() -> DirectionsContainer
{
    auto directions = DirectionsContainer{};

    for (auto i = 0; i < 64; ++i)
    {
        auto const pos = BoardCoordinates{i};

        directions.data[i][0] = detail::make_horizontal(pos);
        directions.data[i][1] = detail::make_vertical(pos);
        directions.data[i][2] = detail::make_negative_diagonal(pos);
        directions.data[i][3] = detail::make_positive_diagonal(pos);
    }

    return directions;
}

constexpr auto make_all_circles() -> CirclesContainer
{
    auto circles = CirclesContainer{};

    for (auto pos = 0; pos < 64; ++pos)
        for (auto radius = 0; radius < 8; ++radius)
            circles.data[pos][radius] = detail::make_circle(BoardCoordinates{pos}, radius);

    return circles;
}

}  // namespace rock
