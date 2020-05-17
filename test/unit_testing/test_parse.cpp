#include "doctest_formatting.h"
#include "example_boards.h"
#include "rock/fen.h"
#include "rock/parse.h"
#include <doctest/doctest.h>

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

static auto test_fen_for_board(rock::Board const& b) -> void
{
    // Formatting boards
    CHECK(rock::parse_fen_to_board(rock::format_as_fen(b)) == b);
    CHECK(rock::parse_fen_to_position(rock::format_as_fen(b)) == std::nullopt);

    // Formatting with white
    {
        auto const pw = rock::Position{b, rock::Player::White};
        CHECK(rock::parse_fen_to_board(rock::format_as_fen(pw)) == b);
        CHECK(rock::parse_fen_to_position(rock::format_as_fen(pw)) == pw);
    }

    // Formatting with black
    {
        auto const pb = rock::Position{b, rock::Player::Black};
        CHECK(rock::parse_fen_to_board(rock::format_as_fen(pb)) == b);
        CHECK(rock::parse_fen_to_position(rock::format_as_fen(pb)) == pb);
    }
}

TEST_CASE("rock::parse_fen")
{
    test_fen_for_board(rock::starting_board);
    for (auto const& board : random_game_boards_5_moves)
        test_fen_for_board(board);
    for (auto const& board : random_game_boards_10_moves)
        test_fen_for_board(board);
    for (auto const& board : assorted_random_game_boards)
        test_fen_for_board(board);
}
