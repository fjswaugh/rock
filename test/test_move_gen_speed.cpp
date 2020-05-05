#include "doctest_formatting.h"
#include "rock/algorithms.h"
#include "rock/starting_position.h"
#include <doctest/doctest.h>
#include <fmt/format.h>
#include <chrono>
#include <iostream>

namespace ch = std::chrono;
using Clock = ch::high_resolution_clock;

// clang-format off
auto const board_0 = rock::starting_board;
auto const board_1 = rock::parse_literal_board(
/*7*/ "        "
/*6*/ "        "
/*5*/ "   w    "
/*4*/ "        "
/*3*/ "        "
/*2*/ "    b   "
/*1*/ "        "
/*0*/ "        ");
     /*01234567*/
// clang-format on

TEST_CASE("rock::count_moves")
{
    CHECK(rock::count_moves({board_0, rock::Player::White}, 1) == 36);
    CHECK(rock::count_moves({board_1, rock::Player::White}, 1) == 8);
    CHECK(rock::count_moves({board_1, rock::Player::White}, 2) == 64);
}

TEST_CASE("rock::list_legal_destinations")
{
    auto destinations = rock::list_legal_destinations({3, 5}, {board_1, rock::Player::White});
    auto expected_destinations = std::vector<rock::BoardCoordinates>{
        {2, 6}, {3, 6}, {4, 6}, {2, 5}, {4, 5}, {2, 4}, {3, 4}, {4, 4}};
    std::sort(destinations.begin(), destinations.end());
    std::sort(expected_destinations.begin(), expected_destinations.end());
    CHECK(destinations == expected_destinations);

    CHECK(
        rock::list_legal_destinations({0, 0}, {board_1, rock::Player::White}) ==
        std::vector<rock::BoardCoordinates>{});
}

TEST_CASE("rock::count_moves")
{
    auto const starting_position = rock::Position{rock::starting_board, rock::Player::White};
    auto const level = 5;

    auto const t_begin = Clock::now();
    auto const n = rock::count_moves(starting_position, level);
    auto const t_end = Clock::now();

    std::cout << fmt::format(
        "rock::count_moves(starting_position, {}) = {} [duration = {}ms]\n",
        level,
        n,
        ch::duration_cast<ch::milliseconds>(t_end - t_begin).count());
}
