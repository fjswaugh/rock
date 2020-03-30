#include <doctest/doctest.h>
#include "rock/parse.h"

TEST_CASE("rock::parse_board_position")
{
    CHECK(rock::parse_board_position("A1") == rock::BoardPosition{0, 0});
}

TEST_CASE("rock::parse_move")
{
    CHECK(
        rock::parse_move("A1 B2") ==
        rock::Move{rock::BoardPosition{0, 0}.data(), rock::BoardPosition{1, 1}.data()});
}
