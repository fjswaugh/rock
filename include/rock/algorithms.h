#pragma once

#include "types.h"
#include <random>

namespace rock
{

auto generate_moves(Board const& board, Color player) -> MoveList;
auto count_moves(Board const& board, Color player_to_move, int level = 0) -> std::size_t;
auto find_all_neighbours_of(u64 pieces, u64 board) -> u64;
auto are_pieces_all_together(u64 const board) -> bool;

template <typename Rng>
auto pick_random_move(Board const& board, Color const player_to_move, Rng& rng) -> Move
{
    auto const moves = generate_moves(board, player_to_move);
    auto dist = std::uniform_int_distribution<std::size_t>{0, moves.size() - 1};
    return moves[dist(rng)];
}

}  // namespace rock
