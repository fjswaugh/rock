#include "rock/algorithms.h"
#include "rock/format.h"
#include "rock/starting_position.h"
#include <doctest/doctest.h>
#include <chrono>

namespace ch = std::chrono;
using Clock = ch::high_resolution_clock;

TEST_CASE("rock::recommend_move_speed")
{
    auto const t_begin = Clock::now();
    auto const [move, score] = rock::recommend_move(rock::starting_position);
    auto const t_end = Clock::now();

    fmt::print(
        "rock::recommend_move(starting_position) = ({}, {}) [duration = {}ms]\n",
        move,
        score,
        ch::duration_cast<ch::milliseconds>(t_end - t_begin).count());
}
