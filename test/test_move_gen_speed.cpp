#include "rock/algorithms.h"
#include "rock/starting_position.h"
#include <doctest/doctest.h>
#include <fmt/ostream.h>
#include <chrono>

namespace ch = std::chrono;
using Clock = ch::high_resolution_clock;

TEST_CASE("rock::generate_moves")
{
    auto const board0 = rock::starting_board;
    auto const board1 = rock::parse_literal_board("        "
                                                  "        "
                                                  "   w    "
                                                  "        "
                                                  "        "
                                                  "    b   "
                                                  "        "
                                                  "        ");

    CHECK(rock::count_moves({board0, rock::Player::White}, 1) == 36);
    CHECK(rock::count_moves({board1, rock::Player::White}, 1) == 8);
    CHECK(rock::count_moves({board1, rock::Player::White}, 2) == 64);
}

TEST_CASE("rock::count_moves")
{
    auto const starting_position = rock::Position{rock::starting_board, rock::Player::White};
    auto const level = 5;

    auto const t_begin = Clock::now();
    auto const n = rock::count_moves(starting_position, level);
    auto const t_end = Clock::now();

    fmt::print(
        "rock::count_moves(starting_position, {}) = {} [duration = {}ms]\n",
        level,
        n,
        ch::duration_cast<ch::milliseconds>(t_end - t_begin).count());
}
