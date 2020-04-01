#include "rock/algorithms.h"
#include "rock/parse.h"
#include "table_generation.h"
#include <algorithm>

namespace rock
{

namespace
{
    constexpr double big = 1000.0;

    template <typename T, std::size_t N>
    using array_ref = T const (&)[N];

    constexpr auto all_circles = make_all_circles();
    constexpr auto all_directions = make_all_directions();

    auto pop_count(u64 x) -> u64 { return __builtin_popcountl(x); }
    auto position_from_board(u64 x) -> u64 { return __builtin_ctzl(x); }
    constexpr auto board_from_position(u8 pos) -> u64 { return u64(1) << u64(pos); }
}  // namespace

auto generate_moves(Board const& board, Player player) -> MoveList
{
    auto const enemy_pieces = board[!player];
    auto const friend_pieces = board[player];
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
                list.push_back({BoardPosition{pos}, BoardPosition{to}});
            }

            if (ce_d_n && (enemy_pieces & c_d_n) == u64{} && (friend_pieces & ce_d_n) == u64{})
            {
                auto const to = position_from_board(ce_d_n);
                list.push_back({BoardPosition{pos}, BoardPosition{to}});
            }
        }
    }

    return list;
}

auto count_moves(Board const& board, Player player_to_move, int level) -> std::size_t
{
    if (level <= 0)
        return 1;

    auto moves = generate_moves(board, player_to_move);

    if (level == 1)
        return moves.size();

    auto num_moves = std::size_t{};

    for (auto const move : moves)
    {
        auto const new_board = apply_move(move, board, player_to_move);
        num_moves += count_moves(new_board, !player_to_move, level - 1);
    }

    return num_moves;
}

auto is_legal_move(Move move, Board const& board, Player player) -> bool
{
    auto const all_moves = generate_moves(board, player);
    return std::find(all_moves.begin(), all_moves.end(), move) != all_moves.end();
}

namespace
{
    auto find_all_neighbours_of(BitBoard pieces, BitBoard board) -> u64
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
}  // namespace

auto are_pieces_all_together(BitBoard const board) -> bool
{
    auto const pos = position_from_board(board);
    auto const pos_board = board_from_position(pos);

    auto const blob = find_all_neighbours_of(pos_board, board);

    return (board ^ blob) == 0;
}

auto get_game_outcome(Board const& board, Player player_to_move) -> GameOutcome
{
    bool const w = are_pieces_all_together(board[Player::White]);
    bool const b = are_pieces_all_together(board[Player::Black]);

    if (w && !b)
        return GameOutcome::WhiteWins;
    if (b && !w)
        return GameOutcome::BlackWins;
    if ((w && b) || count_moves(board, player_to_move) == 0)
        return GameOutcome::Draw;
    return GameOutcome::Ongoing;
}

namespace
{
    constexpr std::pair<BitBoard, double> important_positions[] = {
        {all_circles.data[BoardPosition{3, 3}.data()][3], 1.0},
        {all_circles.data[BoardPosition{3, 3}.data()][2], 1.0},
        {all_circles.data[BoardPosition{3, 3}.data()][1], 1.0},
    };

    auto evaluate_leaf_position(Board const& board, Player player) -> double
    {
        bool const has_player_won = are_pieces_all_together(board[player]);
        bool const has_player_lost = are_pieces_all_together(board[!player]);

        if (has_player_lost || has_player_won)
            return 1000.0 * (double(has_player_won) - double(has_player_lost));

        auto res = double{};

        for (auto const& [positions, value] : important_positions)
        {
            res += value * static_cast<double>(pop_count(positions & board[player]));
            res -= value * static_cast<double>(pop_count(positions & board[!player]));
        }

        return res;
    }

    auto recommend_move_negamax(Board const& board, Player player, int depth) -> MoveRecommendation
    {
        if (depth == 0 || get_game_outcome(board, player) != GameOutcome::Ongoing)
            return {{}, evaluate_leaf_position(board, player)};

        auto best_score = -big;
        auto best_move = Move{};

        for (auto move : generate_moves(board, player))
        {
            auto const new_board = apply_move(move, board, player);
            auto const score = -recommend_move_negamax(new_board, !player, depth - 1).score;

            if (score > best_score)
            {
                best_score = score;
                best_move = move;
            }
        }

        return {best_move, best_score};
    }

    auto recommend_move_negamax_ab(
        Board const& board, Player player, int depth, double alpha, double beta)
        -> MoveRecommendation
    {
        if (depth == 0 || get_game_outcome(board, player) != GameOutcome::Ongoing)
            return {{}, evaluate_leaf_position(board, player)};

        auto best_score = -big;
        auto best_move = Move{};

        for (auto move : generate_moves(board, player))
        {
            auto const new_board = apply_move(move, board, player);
            auto const score =
                -recommend_move_negamax_ab(new_board, !player, depth - 1, -beta, -alpha).score;

            if (score > best_score)
            {
                best_score = score;
                best_move = move;
            }

            if (best_score > alpha)
            {
                // Until this happens, we are an 'All-Node'
                // Now we may be a 'PV-Node', or...
                alpha = best_score;
            }

            if (alpha >= beta)
            {
                // ...if this happens, we are a 'Cut-Node'
                break;
            }
        }

        // Note, we may return values outside of the range [alpha, beta]. This
        // makes us a 'fail-soft' version of alpha-beta pruning
        return {best_move, best_score};
    }

    auto recommend_move_negamax_ab_killer(
        Board const& board,
        Player player,
        int depth,
        double alpha,
        double beta,
        std::optional<Move> killer_move) -> MoveRecommendation
    {
        if (depth == 0 || get_game_outcome(board, player) != GameOutcome::Ongoing)
            return {{}, evaluate_leaf_position(board, player)};

        auto best_score = -big;
        auto best_move = Move{};

        auto all_moves = generate_moves(board, player);

        if (killer_move)
        {
            if (auto it = std::find(all_moves.begin(), all_moves.end(), *killer_move);
                it != all_moves.end())
            {
                std::swap(*all_moves.begin(), *it);
            }

            killer_move = std::nullopt;
        }

        for (auto move : all_moves)
        {
            auto const new_board = apply_move(move, board, player);
            auto const recommendation = recommend_move_negamax_ab_killer(
                new_board, !player, depth - 1, -beta, -alpha, killer_move);

            auto const score = -recommendation.score;

            if (score > best_score)
            {
                best_score = score;
                best_move = move;
                killer_move = recommendation.move;
            }

            if (best_score > alpha)
            {
                // Until this happens, we are an 'All-Node'
                // Now we may be a 'PV-Node', or...
                alpha = best_score;
            }

            if (alpha >= beta)
            {
                // ...if this happens, we are a 'Cut-Node'
                break;
            }
        }

        // Note, we may return values outside of the range [alpha, beta]. This
        // makes us a 'fail-soft' version of alpha-beta pruning
        return {best_move, best_score};
    }
}  // namespace

auto recommend_move(Board const& board, Player player) -> MoveRecommendation
{
    return recommend_move_negamax_ab_killer(board, player, 6, -big, big, {});
}

auto evaluate_position(Board const& board, Player player) -> double
{
    return recommend_move(board, player).score;
}

auto normalize_score(double score, Player player) -> double
{
    return player == Player::Black ? -score : score;
}

}  // namespace rock
