#pragma once

#include "bit_operations.h"
#include "internal_types.h"
#include "rock/algorithms.h"
#include "table_generation.h"

namespace rock::internal
{

inline auto
apply_move_low_level(BitBoard const from, BitBoard const to, BitBoard* mine, BitBoard* theirs)
    -> void
{
    mine->data ^= (from | to);
    theirs->data &= ~to;
}

inline auto apply_move_low_level(Move const m, BitBoard* mine, BitBoard* theirs) -> void
{
    auto const from = m.from.bit_board();
    auto const to = m.to.bit_board();
    return apply_move_low_level(from, to, mine, theirs);
}

inline constexpr ScoreType big = 1'000'000'000ll;

template <typename T, std::size_t N>
using array_ref = T const (&)[N];

inline constexpr auto all_circles = make_all_circles();
inline constexpr auto all_directions = make_all_directions();

inline auto generate_legal_destinations(
    u64 const from_coordinate, BitBoard const friends, BitBoard const enemies) -> u64
{
    assert(bit_board_from_coordinates(from_coordinate) & friends);

    auto result = u64{};

    auto const all_pieces = friends | enemies;

    auto const positive = (~u64{}) << from_coordinate;
    auto const negative = ~positive;

    array_ref<u64, 4> directions = all_directions.data[from_coordinate];
    array_ref<u64, 8> circles = all_circles.data[from_coordinate];

    for (u64 const dir : directions)
    {
        auto const possible_distance = pop_count(dir & all_pieces);

        u64 const circle = circles[possible_distance - 1];
        u64 const circle_edge = circles[std::min(u64{7}, possible_distance)] ^ circle;

        auto const d_p = dir & positive;
        auto const d_n = dir & negative;
        auto const ce_d_p = circle_edge & d_p;
        auto const ce_d_n = circle_edge & d_n;
        auto const c_d_p = circle & d_p;
        auto const c_d_n = circle & d_n;

        assert(pop_count(circle_edge & dir & positive) <= 1);

        if (ce_d_p && (enemies & c_d_p) == u64{} && (friends & ce_d_p) == u64{})
            result |= ce_d_p;

        if (ce_d_n && (enemies & c_d_n) == u64{} && (friends & ce_d_n) == u64{})
            result |= ce_d_n;
    }

    return result;
}

inline auto generate_legal_destinations(BoardCoordinates const from, Position const& position) -> BitBoard
{
    auto const friends = position.board()[position.player_to_move()];
    auto const enemies = position.board()[!position.player_to_move()];

    return generate_legal_destinations(from.data(), friends, enemies);
}

inline auto is_move_legal(InternalMove move, BitBoard const friends, BitBoard const enemies) -> bool
{
    if (!(move.from_board & friends))
        return false;

    auto const from_coordinates = coordinates_from_bit_board(move.from_board);
    return generate_legal_destinations(from_coordinates, friends, enemies) & move.to_board;
}

inline auto generate_moves(BitBoard const friends, BitBoard const enemies) -> InternalMoveList
{
    auto list = InternalMoveList{};
    auto pieces_to_process = friends;
    while (pieces_to_process)
    {
        auto const from_pos = coordinates_from_bit_board(pieces_to_process);
        auto const from_board = bit_board_from_coordinates(from_pos);
        auto const destinations = generate_legal_destinations(from_pos, friends, enemies);
        list.push_back({from_board, destinations});

        pieces_to_process ^= from_board;
    }
    return list;
}

inline auto count_moves(BitBoard const friends, BitBoard const enemies, int level) -> std::size_t
{
    if (level <= 0)
        return 1;

    auto moves = generate_moves(friends, enemies);
    auto num_moves = std::size_t{};

    if (level == 1)
    {
        for (auto const& move_set : moves)
            num_moves += pop_count(move_set.to_board);
    }
    else
    {
        auto const add_to_count = [&](u64 from_board, u64 to_board) {
            auto enemies_copy = enemies;
            auto friends_copy = friends;
            apply_move_low_level(from_board, to_board, &friends_copy, &enemies_copy);

            num_moves += count_moves(enemies_copy, friends_copy, level - 1);
        };
        for_each_move(moves, add_to_count);
    }

    return num_moves;
}

}  // namespace rock::internal
