#include "example_boards.h"
#include "internal/transposition_table.h"
#include "rock/algorithms.h"
#include "rock/format.h"
#include "rock/starting_position.h"
#include <doctest/doctest.h>
#include <chrono>

namespace ch = std::chrono;
using Clock = ch::high_resolution_clock;

TEST_CASE("rock::recommend_move_speed_starting_board")
{
    auto const t_begin = Clock::now();
    auto const [move, score] = rock::recommend_move(rock::starting_position);
    auto const t_end = Clock::now();

    auto const move_str = move ? fmt::format("{}", *move) : "null";

    fmt::print(
        "rock::recommend_move(starting_position) = ({}, {}) [duration = {}ms]\n",
        move_str,
        score,
        ch::duration_cast<ch::milliseconds>(t_end - t_begin).count());
}

TEST_CASE("rock::tt_initialize")
{
    {
        auto const t_begin = Clock::now();
        volatile auto table = rock::internal::TranspositionTable{};
        auto const t_end = Clock::now();

        fmt::print(
            "Initialize transposition table [duration = {}ms]\n",
            ch::duration_cast<ch::milliseconds>(t_end - t_begin).count());
    }

    {
        auto table = rock::internal::TranspositionTable{};
        auto const t_begin = Clock::now();
        table.reset();
        auto const t_end = Clock::now();

        fmt::print(
            "Reset transposition table [duration = {}ms]\n",
            ch::duration_cast<ch::milliseconds>(t_end - t_begin).count());
    }
}

TEST_CASE("rock::recommend_move_speed_boards")
{
    for (auto const& board : assorted_random_game_boards)
    {
        auto const t_begin = Clock::now();
        auto const [move, score] = rock::recommend_move(rock::Position{board, rock::Player::White});
        auto const t_end = Clock::now();

        auto const move_str = move ? fmt::format("{}", *move) : "null";

        fmt::print(
            "rock::recommend_move(starting_position) = ({}, {}) [duration = {}ms]\n",
            move_str,
            score,
            ch::duration_cast<ch::milliseconds>(t_end - t_begin).count());
    }
}
