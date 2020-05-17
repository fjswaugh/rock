#include "common.h"
#include "doctest_formatting.h"
#include "rock/algorithms.h"
#include "rock/starting_position.h"
#include <doctest/doctest.h>
#include <fmt/format.h>

TEST_CASE("rock::count_moves")
{
    for (auto level = 0; level <= 5; ++level)
    {
        auto const t_begin = Clock::now();
        auto const n = rock::count_moves(rock::starting_position, level);
        auto const t_end = Clock::now();

        fmt::print(
            "rock::count_moves(starting_position, {}) = {:10} [duration = {}ms]\n",
            level,
            n,
            ch::duration_cast<ch::milliseconds>(t_end - t_begin).count());
    }
}
