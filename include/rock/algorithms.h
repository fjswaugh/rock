#pragma once

#include "types.h"
#include <vector>

namespace rock
{

auto apply_move(Move, Board, Player) -> Board;
auto apply_move(Move, Position) -> Position;

auto list_moves(Position const&) -> std::vector<Move>;
auto count_moves(Position const&, int level = 1) -> std::size_t;
auto is_legal_move(Move, Position const&) -> bool;
auto list_legal_destinations(BoardCoordinates from, Position const&) -> std::vector<BoardCoordinates>;

auto are_pieces_all_together(BitBoard) -> bool;
auto get_game_outcome(Position const&) -> GameOutcome;

struct MoveRecommendation
{
    Move move;
    double score;
};

/**
 * Player to move, positive result is good for player
 */
auto recommend_move(Position const&) -> MoveRecommendation;
auto evaluate_position(Position const&) -> double;

/**
 * Normalize a score generated by the above functions so that positive always
 * means that White is winning
 */
auto normalize_score(double score, Player player) -> double;

}  // namespace rock
