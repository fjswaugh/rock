#include "rock/algorithms.h"
#include "table_generation.h"

namespace rock
{

template <typename T, std::size_t N>
using array_ref = T const (&)[N];

constexpr auto all_circles = make_all_circles();
constexpr auto all_directions = make_all_directions();

auto pop_count(u64 x) -> u64
{
    return __builtin_popcountl(x);
}

auto position_from_board(u64 x) -> u64
{
    return __builtin_ctzl(x);
}

constexpr auto board_from_position(u8 pos) -> u64
{
    return u64(1) << u64(pos);
}

auto generate_moves(Board const& board, Color player) -> MoveList
{
    auto const enemy_pieces = board.pieces[!bool(player)];
    auto const friend_pieces = board.pieces[bool(player)];
    auto const all_pieces = enemy_pieces | friend_pieces;

    auto list = MoveList{};

    auto pieces_to_process = friend_pieces;
    while (pieces_to_process)
    {
        auto const pos = position_from_board(pieces_to_process);
        pieces_to_process ^= board_from_position(pos);

        auto const positive = (~u64{}) << u64(pos);
        auto const negative = ~positive;

        array_ref<u64, 4> directions = all_directions.data[pos];
        array_ref<u64, 8> circles = all_circles.data[pos];

        for (u64 const dir : directions)
        {
            auto const possible_distance = pop_count(dir & all_pieces);

            u64 const circle = circles[possible_distance - 1];
            u64 const circle_edge = circles[std::min(u64(7), possible_distance)] ^ circle;

            auto const d_p = dir & positive;
            auto const d_n = dir & negative;
            auto const ce_d_p = circle_edge & d_p;
            auto const ce_d_n = circle_edge & d_n;
            auto const c_d_p = circle & d_p;
            auto const c_d_n = circle & d_n;

            assert(pop_count(circle_edge & dir & positive) <= 1);

            if (ce_d_p && (enemy_pieces & c_d_p) == u64{} && (friend_pieces & ce_d_p) == u64{})
            {
                auto const to = position_from_board(ce_d_p);
                list.push_back({u8(pos), u8(to)});
            }

            if (ce_d_n && (enemy_pieces & c_d_n) == u64{} && (friend_pieces & ce_d_n) == u64{})
            {
                auto const to = position_from_board(ce_d_n);
                list.push_back({u8(pos), u8(to)});
            }
        }
    }

    return list;
}

auto count_moves(Board const& board, Color player_to_move, int level) -> std::size_t
{
    if (level <= 0)
        return 1;

    auto moves = generate_moves(board, player_to_move);

    if (level == 1)
        return moves.size();

    auto num_moves = int{};

    for (auto const move : moves)
    {
        auto const new_board = apply_move(move, board, player_to_move);
        auto const new_player = Color{!bool(player_to_move)};
        num_moves += count_moves(new_board, new_player, level - 1);
    }

    return num_moves;
}

auto find_all_neighbours_of(u64 pieces, u64 board) -> u64
{
    auto found = u64{};

    while (pieces)
    {
        auto const pos = position_from_board(pieces);
        auto const pos_board = board_from_position(pos);

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

auto are_pieces_all_together(u64 const board) -> bool
{
    auto const pos = position_from_board(board);
    auto const pos_board = board_from_position(pos);

    auto const blob = find_all_neighbours_of(pos_board, board);

    return (board ^ blob) == 0;
}

}  // namespace rock
