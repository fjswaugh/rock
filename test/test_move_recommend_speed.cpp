#include "rock/algorithms.h"
#include "rock/format.h"
#include "rock/starting_position.h"
#include <doctest/doctest.h>
#include <chrono>

namespace ch = std::chrono;
using Clock = ch::high_resolution_clock;

static constexpr rock::Board test_boards[] = {
    rock::parse_literal_board("b     bw"
                              " w      "
                              "   w    "
                              "    b   "
                              "    b   "
                              "  bw w  "
                              " wbw  bb"
                              "bb      "),
    rock::parse_literal_board(" w   ww "
                              "       b"
                              "      wb"
                              "b      b"
                              "b      b"
                              "bbw w  w"
                              "b      b"
                              " wbw  w "),
    rock::parse_literal_board(" w  www "
                              "b    b  "
                              " w w   b"
                              "  b    b"
                              "b   b  b"
                              " w     b"
                              "b      b"
                              " wwwww  "),
    rock::parse_literal_board("bw w  w "
                              "  bb    "
                              "   ww  w"
                              "  w    b"
                              "bb  b  b"
                              " w  w   "
                              "        "
                              "     bwb"),
    rock::parse_literal_board("ww ww   "
                              "bb     w"
                              "  b  w  "
                              "        "
                              "     bwb"
                              "w   w  b"
                              "  w b   "
                              "    b b "),
    rock::parse_literal_board(" www ww "
                              "b    w b"
                              "      b "
                              "   b b  "
                              " b  bw b"
                              "w     w "
                              "  bwb   "
                              "   w w  "),
    rock::parse_literal_board(" www  w "
                              "b    b b"
                              "bw     b"
                              "     b  "
                              " w     b"
                              "  b  ww "
                              "bw      "
                              "w  bww  "),
    rock::parse_literal_board(" w    ww"
                              " b  b   "
                              "  b w  b"
                              "b      b"
                              "b      b"
                              "    w ww"
                              " b     b"
                              " bww  w "),
};

TEST_CASE("rock::recommend_move_speed_starting_board")
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

TEST_CASE("rock::recommend_move_speed_boards")
{
    for (auto const& board : test_boards)
    {
        auto const t_begin = Clock::now();
        auto const [move, score] = rock::recommend_move(rock::Position{board, rock::Player::White});
        auto const t_end = Clock::now();

        fmt::print(
            "rock::recommend_move(starting_position) = ({}, {}) [duration = {}ms]\n",
            move,
            score,
            ch::duration_cast<ch::milliseconds>(t_end - t_begin).count());
    }
}
