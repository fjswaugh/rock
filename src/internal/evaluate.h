#pragma once

#include "internal_types.h"
#include "move_generation.h"
#include "rock/parse.h"

namespace rock::internal
{

inline auto find_all_neighbours_of(BitBoard pieces, BitBoard board) -> u64
{
    auto found = u64{};

    while (pieces)
    {
        auto const pos = coordinates_from_bit_board(pieces);
        auto const pos_board = bit_board_from_coordinates(pos);

        auto const circle = all_circles.data[pos][1];
        auto const edge = circle ^ pos_board;

        auto const populated_circle = circle & board;
        auto const populated_edge = edge & board;

        found |= pos_board;
        found |= populated_circle;
        found |= find_all_neighbours_of(populated_edge, board & (~found));

        pieces &= (~found);
    }

    return found;
}

inline auto are_pieces_all_together(BitBoard const board) -> bool
{
    auto const pos = coordinates_from_bit_board(board);
    auto const pos_board = bit_board_from_coordinates(pos);

    auto const blob = find_all_neighbours_of(pos_board, board);

    return (board ^ blob) == 0;
}

inline constexpr BitBoard central_boards[] = {
    parse_literal_bit_board("        "
                            "        "
                            "        "
                            "   xx   "
                            "   xx   "
                            "        "
                            "        "
                            "        "),
    parse_literal_bit_board("        "
                            "        "
                            "  xxxx  "
                            "  xxxx  "
                            "  xxxx  "
                            "  xxxx  "
                            "        "
                            "        "),
    parse_literal_bit_board("        "
                            " xxxxxx "
                            " xxxxxx "
                            " xxxxxx "
                            " xxxxxx "
                            " xxxxxx "
                            " xxxxxx "
                            "        "),
};

inline constexpr std::pair<BitBoard, ScoreType> important_positions[] = {
    {central_boards[0], 10},
    {central_boards[1], 10},
    {central_boards[2], 10},
};

inline auto evaluate_leaf_position(
    BitBoard friends, BitBoard enemies, bool has_player_won, bool has_player_lost) -> ScoreType
{
    auto res = ScoreType{};

    if (has_player_lost || has_player_won)
        res += big * static_cast<ScoreType>(has_player_won - has_player_lost);

    for (auto const& [positions, value] : important_positions)
    {
        res += value * static_cast<ScoreType>(pop_count(positions & friends));
        res -= value * static_cast<ScoreType>(pop_count(positions & enemies));
    }

    res += 20;

    return res;
}

inline auto evaluate_leaf_position(BitBoard friends, BitBoard enemies) -> ScoreType
{
    bool const has_player_won = are_pieces_all_together(friends);
    bool const has_player_lost = are_pieces_all_together(enemies);

    return evaluate_leaf_position(friends, enemies, has_player_won, has_player_lost);
}

}  // namespace rock::internal
