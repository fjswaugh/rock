//#define NO_USE_TT
//#define NO_USE_KILLER
//#define NO_USE_NEGASCOUT
//#define DIAGNOSTICS
//#define NO_USE_CUSTOM_TT

#define DEPTH 8

#include "rock/algorithms.h"
#include "bit_operations.h"
#include "diagnostics.h"
#include "internal_types.h"
#include "rock/format.h"
#include "rock/parse.h"
#include "table_generation.h"
#include "transposition_table.h"
#include <fmt/format.h>
#include <algorithm>
#include <iostream>
#include <random>

#ifndef NO_USE_TT

#include <absl/container/flat_hash_map.h>
#include <absl/hash/hash.h>
#include <memory>

#endif

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

////////////////////////////////////////////////////////////////////////////////////////////////////
// Apply move
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace
{
    auto
    apply_move_low_level(BitBoard const from, BitBoard const to, BitBoard* mine, BitBoard* theirs)
        -> void
    {
        mine->data ^= (from | to);
        theirs->data &= ~to;
    }

    auto apply_move_low_level(Move const m, BitBoard* mine, BitBoard* theirs) -> void
    {
        auto const from = m.from.bit_board();
        auto const to = m.to.bit_board();
        return apply_move_low_level(from, to, mine, theirs);
    }
}  // namespace

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

////////////////////////////////////////////////////////////////////////////////////////////////////
// Function implementations
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace
{
    constexpr ScoreType big = 1'000'000'000ll;

    template <typename T, std::size_t N>
    using array_ref = T const (&)[N];

    constexpr auto all_circles = make_all_circles();
    constexpr auto all_directions = make_all_directions();

    auto generate_legal_destinations(
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

    auto generate_legal_destinations(BoardCoordinates const from, Position const& position)
        -> BitBoard
    {
        auto const friends = position.board()[position.player_to_move()];
        auto const enemies = position.board()[!position.player_to_move()];

        return generate_legal_destinations(from.data(), friends, enemies);
    }

    auto is_move_legal(InternalMove move, BitBoard const friends, BitBoard const enemies) -> bool
    {
        if (!(move.from_board & friends))
            return false;

        auto const from_coordinates = coordinates_from_bit_board(move.from_board);
        return generate_legal_destinations(from_coordinates, friends, enemies) & move.to_board;
    }

    auto generate_moves(BitBoard const friends, BitBoard const enemies) -> InternalMoveList
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

    auto count_moves(BitBoard const friends, BitBoard const enemies, int level) -> std::size_t
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
}  // namespace

////////////////////////////////////////////////////////////////////////////////////////////////////
// Main functions
////////////////////////////////////////////////////////////////////////////////////////////////////

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

auto is_legal_move(Move move, Position const& position) -> bool
{
    auto const all_moves = list_moves(position);
    return std::find(all_moves.begin(), all_moves.end(), move) != all_moves.end();
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

namespace
{
    auto find_all_neighbours_of(BitBoard pieces, BitBoard board) -> u64
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
}  // namespace

auto are_pieces_all_together(BitBoard const board) -> bool
{
    auto const pos = coordinates_from_bit_board(board);
    auto const pos_board = bit_board_from_coordinates(pos);

    auto const blob = find_all_neighbours_of(pos_board, board);

    return (board ^ blob) == 0;
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

////////////////////////////////////////////////////////////////////////////////////////////////////
// Move recommendation algorithms (alpha-beta search etc.)
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace
{
    constexpr std::pair<BitBoard, ScoreType> important_positions[] = {
        {all_circles.data[BoardCoordinates{3, 3}.data()][3], 10},
        {all_circles.data[BoardCoordinates{3, 3}.data()][2], 10},
        {all_circles.data[BoardCoordinates{3, 3}.data()][1], 10},
    };

    auto evaluate_leaf_position(
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

        return res;
    }

    auto evaluate_leaf_position(BitBoard friends, BitBoard enemies) -> ScoreType
    {
        bool const has_player_won = are_pieces_all_together(friends);
        bool const has_player_lost = are_pieces_all_together(enemies);

        return evaluate_leaf_position(friends, enemies, has_player_won, has_player_lost);
    }

#ifndef NO_USE_TT
    TranspositionTable table{};
    int pv_count{};
#endif

    struct Searcher
    {
#ifndef NO_USE_KILLER
        explicit Searcher(
            BitBoard friends,
            BitBoard enemies,
            int depth,
            ScoreType alpha,
            ScoreType beta,
            InternalMove killer_move = {});
#else
        explicit Searcher(
            BitBoard friends, BitBoard enemies, int depth, ScoreType alpha, ScoreType beta);
#endif

        auto search() -> InternalMoveRecommendation;

    private:
        // Internal functions
        auto search_next(BitBoard friends, BitBoard enemies, ScoreType alpha, ScoreType beta)
            -> InternalMoveRecommendation;
        auto main_search() -> void;
        auto process_move(InternalMove) -> void;
        auto add_to_transposition_table() -> void;

        // Input arguments
        BitBoard friends_;
        BitBoard enemies_;
        int depth_;
        ScoreType alpha_;
        ScoreType beta_;
#ifndef NO_USE_KILLER
        InternalMove killer_move_;
#endif

        // Internal data
#ifndef NO_USE_KILLER
        InternalMove next_killer_move_{};
#endif
        InternalMoveRecommendation best_result_;
        NodeType node_type_;
        int move_count_{};

#ifdef DIAGNOSTICS
        Diagnostics::Scratchpad scratchpad_{};
#endif
    };

#ifndef NO_USE_KILLER
    Searcher::Searcher(
        BitBoard friends,
        BitBoard enemies,
        int depth,
        ScoreType alpha,
        ScoreType beta,
        InternalMove killer_move)
        : friends_{friends},
          enemies_{enemies},
          depth_{depth},
          alpha_{alpha},
          beta_{beta},
          killer_move_{killer_move}
    {}
#else
    Searcher::Searcher(
        BitBoard friends, BitBoard enemies, int depth, ScoreType alpha, ScoreType beta)
        : friends_{friends}, enemies_{enemies}, depth_{depth}, alpha_{alpha}, beta_{beta}
    {}
#endif

    auto Searcher::search_next(BitBoard friends, BitBoard enemies, ScoreType alpha, ScoreType beta)
        -> InternalMoveRecommendation
    {
#ifndef NO_USE_KILLER
        auto searcher = Searcher(friends, enemies, depth_ - 1, alpha, beta, next_killer_move_);
#else
        auto searcher = Searcher(friends, enemies, depth_ - 1, alpha, beta);
#endif
        return searcher.search();
    }

    auto Searcher::search() -> InternalMoveRecommendation
    {
        if (depth_ == 0)
        {
            return {InternalMove{}, evaluate_leaf_position(friends_, enemies_)};
        }
        else
        {
            main_search();
            DIAGNOSTICS_UPDATE_AFTER_SEARCH(best_result_, move_count_);
            add_to_transposition_table();
            return best_result_;
        }
    }

    auto Searcher::process_move(InternalMove move) -> void
    {
        auto friends_copy = friends_;
        auto enemies_copy = enemies_;
        apply_move_low_level(move.from_board, move.to_board, &friends_copy, &enemies_copy);

        InternalMoveRecommendation recommendation;
        ScoreType score;

#ifndef NO_USE_NEGASCOUT
        if (move_count_ > 0)
        {
            recommendation = search_next(enemies_copy, friends_copy, -alpha_ - 1, -alpha_);
            score = -recommendation.score;

            bool const must_re_search = score > alpha_ && score < beta_;

            if (must_re_search)
            {
                recommendation = search_next(enemies_copy, friends_copy, -beta_, -score);
                score = -recommendation.score;
            }

            DIAGNOSTICS_UPDATE(negascout_re_search, must_re_search);
        }
        else
#endif
        {
            recommendation = search_next(enemies_copy, friends_copy, -beta_, -alpha_);
            score = -recommendation.score;
        }

        if (score > best_result_.score)
        {
            best_result_.move = move;
            best_result_.score = score;
#ifndef NO_USE_KILLER
            next_killer_move_ = recommendation.move;
#endif
        }

        if (best_result_.score > alpha_)
        {
            // Until this happens, we are an 'All-Node'
            // Now we may be a 'PV-Node', or...
            alpha_ = best_result_.score;
            node_type_ = NodeType::Pv;
        }

        if (alpha_ >= beta_)
        {
            // ...if this happens, we are a 'Cut-Node'
            node_type_ = NodeType::Cut;
        }

        DIAGNOSTICS_UPDATE_AFTER_MOVE(node_type_, score);

        ++move_count_;
    }

    auto Searcher::main_search() -> void
    {
        DIAGNOSTICS_UPDATE_BEFORE_SEARCH();

        best_result_ = InternalMoveRecommendation{InternalMove{}, -big};
        node_type_ = NodeType::All;

#ifndef NO_USE_TT
        // Check the transposition table before checking if the game is over
        // (this works out faster)
        auto const [tt_ptr, was_found] = table.lookup(friends_, enemies_);
        auto tt_move = InternalMove{};
        DIAGNOSTICS_UPDATE(tt_had_move_cached, was_found);
        if (was_found)
        {
            tt_move = tt_ptr->recommendation.move;

            bool const tt_is_exact_match = tt_move.empty() ||
                (tt_ptr->type == NodeType::Pv && tt_ptr->depth >= depth_);

            DIAGNOSTICS_UPDATE(tt_move_is_exact_match, tt_is_exact_match);
            if (tt_is_exact_match)
            {
                best_result_ = tt_ptr->recommendation;
                return;
            }

            DIAGNOSTICS_PREPARE_TT_MOVE();
            this->process_move(tt_move);
            if (node_type_ == NodeType::Cut)
                return;
        }
#endif

#ifndef NO_USE_KILLER
        if (!killer_move_.empty() && is_move_legal(killer_move_, friends_, enemies_))
        {
            DIAGNOSTICS_PREPARE_KILLER_MOVE();
            this->process_move(killer_move_);
            if (node_type_ == NodeType::Cut)
                return;
        }
#endif

        auto moves = generate_moves(friends_, enemies_);

        // if game is over, return early
        {
            bool const has_player_won = are_pieces_all_together(friends_);
            bool const has_player_lost = are_pieces_all_together(enemies_);
            if (moves.size() == 0 || has_player_won || has_player_lost)
            {
                best_result_.move = InternalMove{};
                best_result_.score =
                    evaluate_leaf_position(friends_, enemies_, has_player_won, has_player_lost);
                return;
            }
        }

        for (auto move_set : moves)
        {
            while (move_set.to_board)
            {
                auto const to_board = extract_one_bit(move_set.to_board);
                auto const move = InternalMove{move_set.from_board, to_board};

#ifndef NO_USE_KILLER
                if (killer_move_ == move)
                    continue;
#endif

#ifndef NO_USE_TT
                if (tt_move == move)
                    continue;
#endif

                this->process_move(move);
                if (node_type_ == NodeType::Cut)
                    return;
            }
        }

        // Note, we may return values outside of the range [alpha, beta] (if we
        // are an 'all' node and score below alpha). This makes us a 'fail-soft'
        // version of alpha-beta pruning.
    }

    auto Searcher::add_to_transposition_table() -> void
    {
#ifndef NO_USE_TT
        auto const [tt_ptr, was_found] = table.lookup(friends_, enemies_);

        bool const are_we_pv = node_type_ == NodeType::Pv;
        bool const are_tt_pv = tt_ptr->type == NodeType::Pv;

        if ((!are_we_pv && !are_tt_pv && depth_ > tt_ptr->depth) ||
            (are_we_pv && (!are_tt_pv || depth_ > tt_ptr->depth)))
        {
            tt_ptr->set_key(friends_, enemies_);
            tt_ptr->recommendation = best_result_;
            tt_ptr->depth = depth_;
            tt_ptr->type = node_type_;
        }
#endif
    }

}  // namespace

auto recommend_move(Position const& position) -> MoveRecommendation
{
#ifndef NO_USE_TT
    table.reset();
    pv_count = {};
#endif
#ifdef DIAGNOSTICS
    diagnostics = {};
#endif
    auto const friends = position.board()[position.player_to_move()];
    auto const enemies = position.board()[!position.player_to_move()];

    auto internal = InternalMoveRecommendation{};

    for (auto depth = 1; depth <= DEPTH; ++depth)
        internal = Searcher(friends, enemies, depth, -big, big).search();

#ifndef NO_USE_TT
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
