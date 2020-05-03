//#define NO_USE_NEGASCOUT
//#define DIAGNOSTICS
//#define NO_USE_CUSTOM_TT

#define DEPTH 8

#include "rock/algorithms.h"
#include "internal/bit_operations.h"
#include "internal/diagnostics.h"
#include "internal/evaluate.h"
#include "internal/internal_types.h"
#include "internal/move_generation.h"
#include "internal/search.h"
#include "internal/table_generation.h"
#include "internal/transposition_table.h"
#include "rock/format.h"
#include "rock/parse.h"
#include <fmt/format.h>
#include <algorithm>
#include <iostream>
#include <random>

// TODO:
// - Consider using strong types more, instead of lots of u64s
//   - Reconsider integer type used to store board position if this is done
//     - No longer needs to be compact (may perform better if not)
// - Should probably work out some performance regression testing
// - Consider better algorithm for detemining if game is over
//   - This check is probably done too many times in the search code
// - Create some kind of stateful AI object to remember information
// - Create some kind of analysis of what is going on in the search tree
// - Create interface to take advantage of iterative deepening

namespace rock
{

using namespace internal;

auto apply_move(Move const m, Board b, Player const player) -> Board
{
    apply_move_low_level(m, &b[player], &b[!player]);
    return b;
}

auto apply_move(Move const m, Position p) -> Position
{
    p.set_board(apply_move(m, p.board(), p.player_to_move()));
    p.set_player_to_move(!p.player_to_move());
    return p;
}

auto list_moves(Position const& position) -> std::vector<Move>
{
    auto result = std::vector<Move>{};

    auto const friends = position.board()[position.player_to_move()];
    auto const enemies = position.board()[!position.player_to_move()];
    auto moves = generate_moves(friends, enemies);

    for_each_move(moves, [&](u64 from_board, u64 to_board) {
        result.push_back({
            BoardCoordinates{coordinates_from_bit_board(from_board)},
            BoardCoordinates{coordinates_from_bit_board(to_board)},
        });
    });

    return result;
}

auto count_moves(Position const& position, int level) -> std::size_t
{
    return count_moves(
        position.board()[position.player_to_move()],
        position.board()[!position.player_to_move()],
        level);
}

auto is_move_legal(Move move, Position const& position) -> bool
{
    return is_move_legal(
        InternalMove::from(move),
        position.board()[position.player_to_move()],
        position.board()[!position.player_to_move()]);
}

auto list_legal_destinations(BoardCoordinates from, Position const& position)
    -> std::vector<BoardCoordinates>
{
    auto res = std::vector<BoardCoordinates>{};
    res.reserve(8);

    if (!(from.bit_board() & position.board()[position.player_to_move()]))
        return res;

    auto destinations = generate_legal_destinations(from, position);

    while (destinations)
    {
        auto const pos = coordinates_from_bit_board(destinations);
        destinations ^= bit_board_from_coordinates(pos);

        res.emplace_back(pos);
    }

    return res;
}

auto get_game_outcome(Position const& position) -> GameOutcome
{
    bool const w = are_pieces_all_together(position.board()[Player::White]);
    bool const b = are_pieces_all_together(position.board()[Player::Black]);

    if (w && !b)
        return GameOutcome::WhiteWins;
    if (b && !w)
        return GameOutcome::BlackWins;
    if ((w && b) || count_moves(position) == 0)
        return GameOutcome::Draw;
    return GameOutcome::Ongoing;
}

static auto table = TranspositionTable{};

auto recommend_move(Position const& position) -> MoveRecommendation
{
    table.reset();
#ifdef DIAGNOSTICS
    diagnostics = {};
#endif
    auto const friends = position.board()[position.player_to_move()];
    auto const enemies = position.board()[!position.player_to_move()];

    auto internal = InternalMoveRecommendation{};

    for (auto depth = 1; depth <= DEPTH; ++depth)
        internal = Searcher(friends, enemies, depth, -big, big, &table).search();

#ifdef DIAGNOSTICS
    auto pv = std::vector<Move>{};

    auto f = friends;
    auto e = enemies;
    while (true)
    {
        auto const [value, was_found] = table.lookup(f, e);
        if (!was_found || value->type != NodeType::Pv)
            break;
        auto const& m = value->recommendation.move;
        if (m.empty())
            break;
        pv.push_back(m.to_standard_move().value());
        apply_move_low_level(m.from_board, m.to_board, &f, &e);
        std::swap(f, e);
    }

    std::cout << fmt::format("Pv: [{}]\n", fmt::join(pv.begin(), pv.end(), ", "));
#endif

#ifdef DIAGNOSTICS
    std::cout << diagnostics.to_string() << '\n';
#endif

    return internal.to_standard_move_recommendation();
}

auto evaluate_position(Position const& position) -> ScoreType
{
    return recommend_move(position).score;
}

auto normalize_score(ScoreType const score, Player const player) -> ScoreType
{
    return player == Player::Black ? -score : score;
}

}  // namespace rock
