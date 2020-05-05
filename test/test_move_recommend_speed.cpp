#include "example_boards.h"
#include "internal/transposition_table.h"
#include "rock/algorithms.h"
#include "rock/format.h"
#include "rock/starting_position.h"
#include <doctest/doctest.h>
#include <chrono>
#include <iostream>

namespace ch = std::chrono;
using Clock = ch::high_resolution_clock;

TEST_CASE("rock::recommend_move_speed_starting_board")
{
    auto const t_begin = Clock::now();
    auto const analysis = rock::analyze_position(rock::starting_position, /*depth=*/8);
    auto const t_end = Clock::now();

    auto const move_str = analysis.best_move ? fmt::format("{}", *analysis.best_move) : "null";

    fmt::print(
        "rock::analyze_position(starting_position) = ({}, {:5}) [duration = {:4}ms] [{}]\n",
        move_str,
        analysis.score,
        ch::duration_cast<ch::milliseconds>(t_end - t_begin).count(),
        fmt::join(analysis.principal_variation.begin(), analysis.principal_variation.end(), ", "));
}

TEST_CASE("rock::recommend_move_speed_boards")
{
    for (auto const& board : assorted_random_game_boards)
    {
        auto const t_begin = Clock::now();
        auto const analysis =
            rock::analyze_position(rock::Position{board, rock::Player::White}, /*depth=*/8);
        auto const t_end = Clock::now();

        auto const move_str = analysis.best_move ? fmt::format("{}", *analysis.best_move) : "null";

        fmt::print(
            "rock::analyze_position(various_positions) = ({}, {:5}) [duration = {:4}ms] [{}]\n",
            move_str,
            analysis.score,
            ch::duration_cast<ch::milliseconds>(t_end - t_begin).count(),
            fmt::join(
                analysis.principal_variation.begin(), analysis.principal_variation.end(), ", "));
    }
}
