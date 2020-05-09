#include "doctest_formatting.h"
#include "rock/algorithms.h"
#include "rock/starting_position.h"
#include <doctest/doctest.h>
#include <fmt/format.h>
#include <chrono>
#include <iostream>

namespace
{

// clang-format off
auto const test_board_0 = rock::parse_literal_board(
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

auto const test_position_0 = rock::Position{test_board_0, rock::Player::White};

}  // namespace

TEST_CASE("rock::count_moves")
{
    CHECK(rock::count_moves(test_position_0, 0) == 1);
    CHECK(rock::count_moves(test_position_0, 1) == 8);
    CHECK(rock::count_moves(test_position_0, 2) == 64);

    CHECK(rock::count_moves(rock::starting_position, 0) == 1);
    CHECK(rock::count_moves(rock::starting_position, 1) == 36);
    CHECK(rock::count_moves(rock::starting_position, 2) == 1244);

    // All other results in this function I have manually verified, but the
    // following one is just the output of the program (at least we can see if
    // it ever changes)
    CHECK(rock::count_moves(rock::starting_position, 5) == 55'963'132);
}

TEST_CASE("rock::list_legal_destinations")
{
    auto destinations = rock::list_legal_destinations({3, 5}, test_position_0);
    auto expected_destinations = std::vector<rock::BoardCoordinates>{
        {2, 6}, {3, 6}, {4, 6}, {2, 5}, {4, 5}, {2, 4}, {3, 4}, {4, 4}};
    std::sort(destinations.begin(), destinations.end());
    std::sort(expected_destinations.begin(), expected_destinations.end());
    CHECK(destinations == expected_destinations);

    CHECK(rock::list_legal_destinations({0, 0}, test_position_0).empty());
}
