#include "common.h"
#include "doctest_formatting.h"
#include "example_boards.h"
#include <doctest/doctest.h>
#include <fmt/format.h>
#include <rock/algorithms.h>
#include <rock/starting_position.h>

namespace
{

auto do_speed_run(rock::Position const& position, std::string_view position_description) -> void
{
    auto const depth = 8;

    auto const t_begin = Clock::now();
    auto const analysis = rock::analyze_position(position, depth);
    auto const t_end = Clock::now();

    auto const move_str = analysis.best_move ? fmt::format("{}", *analysis.best_move) : "null";
    auto const score_str = std::abs(analysis.score) > 100'000
        ? (analysis.score < 0 ? "-" : "+") + std::string{"BIG"}
        : std::to_string(analysis.score);

    fmt::print(
        "rock::analyze_position({:20}) = ({}, {:>5}) [duration = {:4}ms] [{}]\n",
        position_description,
        move_str,
        score_str,
        ch::duration_cast<ch::milliseconds>(t_end - t_begin).count(),
        fmt::join(analysis.principal_variation.begin(), analysis.principal_variation.end(), ", "));
}

template <typename Boards>
auto do_speed_run(Boards const& boards, std::string_view position_description) -> void
{
    auto i = int{};
    for (auto const& board : boards)
    {
        auto const specific_description = fmt::format("{}-{}", position_description, i++);
        do_speed_run(rock::Position{board, rock::Player::White}, specific_description);
    }
}

}  // namespace

TEST_CASE("rock::analyze_starting_position")
{
    do_speed_run(rock::starting_position, "starting_position");
}

TEST_CASE("rock::analyze_associated_positions")
{
    do_speed_run(assorted_random_game_boards, "random_positions");
}

TEST_CASE("rock::analyze_early_positions")
{
    do_speed_run(random_game_boards_5_moves, "early_positions");
}

TEST_CASE("rock::analyze_midgame_positions")
{
    do_speed_run(random_game_boards_10_moves, "midgame_positions");
}
