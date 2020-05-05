#include <doctest/doctest.h>
#include "rock/parse.h"

TEST_CASE("rock::parse_board_position")
{
    CHECK(rock::parse_board_coordinates("A1") == rock::BoardCoordinates{0, 0});
}

TEST_CASE("rock::parse_move")
{
    CHECK(
        rock::parse_move("A1 B2") ==
        rock::Move{rock::BoardCoordinates{0, 0}, rock::BoardCoordinates{1, 1}});
}
