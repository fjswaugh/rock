#include "rock/algorithms.h"
#include "bit_operations.h"
#include "rock/format.h"
#include "rock/parse.h"
#include "table_generation.h"
#include <fmt/format.h>
#include <algorithm>
#include <iostream>
#include <random>

#define USE_NEW
#define USE_TT
#define USE_CUSTOM_TT
#define USE_ID
#define USE_KH
#define USE_NS
#define DIAGNOSTICS

#define DEPTH 8

#ifdef USE_TT

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
// Internal structures for more efficiency
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace
{
    /**
     * Internal move is a more flexible version of the normal move type. The move is represented in
     * terms of bitboards, which allows multiple moves to be effectively stored in the same object,
     * and also allows for an 'empty' move state.
     */
    struct InternalMove
    {
        u64 from_board;
        u64 to_board;

        friend constexpr auto operator==(InternalMove const& m1, InternalMove const& m2) -> bool
        {
            return m1.from_board == m2.from_board && m1.to_board == m2.to_board;
        }
        friend constexpr auto operator!=(InternalMove const& m1, InternalMove const& m2) -> bool
        {
            return m1.from_board != m2.from_board || m1.to_board != m2.to_board;
        }
        constexpr auto empty() const -> bool { return from_board == u64{} && to_board == u64{}; }

        auto to_standard_move() const -> std::optional<Move>
        {
            if (empty())
                return std::nullopt;
            return Move{
                BoardCoordinates{coordinates_from_bit_board(from_board)},
                BoardCoordinates{coordinates_from_bit_board(to_board)},
            };
        }
    };

    struct InternalMoveRecommendation
    {
        InternalMove move;
        ScoreType score;

        auto to_standard_move_recommendation() const -> MoveRecommendation
        {
            return {move.to_standard_move(), score};
        }
    };

    /**
     * The purpose of `InternalMoveList` is to provide an efficient way to store generated moves.
     */
    struct InternalMoveList
    {
        void push_back(InternalMove move_set)
        {
            assert(pop_count(move_set.from_board) == 1);
            assert(size_ < max_size);
            moves_[size_++] = move_set;
        }
        constexpr auto begin() const -> InternalMove const* { return &moves_[0]; }
        constexpr auto end() const -> InternalMove const* { return this->begin() + size_; }
        constexpr auto begin() -> InternalMove* { return &moves_[0]; }
        constexpr auto end() -> InternalMove* { return this->begin() + size_; }
        constexpr auto size() const { return size_; }

    private:
        constexpr static auto max_size = 12;

        InternalMove moves_[max_size];
        std::size_t size_{};
    };

    template <typename F>
    auto for_each_move(InternalMoveList& moves, F&& f) -> void
    {
        for (auto move_set : moves)
        {
            while (move_set.to_board)
            {
                auto const to_board = extract_one_bit(move_set.to_board);
                f(move_set.from_board, to_board);
            }
        }
    }
}  // namespace

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

    enum struct NodeType
    {
        All,
        Pv,
        Cut,
    };

#ifdef DIAGNOSTICS
    struct Boolean
    {
        u64 yes{};
        u64 total{};

        auto update(bool is_yes) -> void
        {
            if (is_yes)
                ++yes;
            ++total;
        }

        auto to_string() const -> std::string
        {
            auto const percent = 100.0 * static_cast<double>(yes) / static_cast<double>(total);
            return fmt::format("{} / {} ({}%)", yes, total, percent);
        }
    };

    struct Number
    {
        u64 sum;
        u64 count;

        auto update(u64 num) -> void
        {
            sum += num;
            ++count;
        }

        auto to_string() const -> std::string
        {
            auto const mean = static_cast<double>(sum) / static_cast<double>(count);
            return fmt::format("mean = {} (count = {})", mean, count);
        }
    };

    struct Diagnostics
    {
        struct Scratchpad
        {
            bool processing_tt_move{};
            bool processing_killer_move{};
            std::optional<ScoreType> tt_move_score{};
            std::optional<ScoreType> killer_move_score{};
            std::optional<ScoreType> first_move_score{};
        };

        // If a certain move is 'best' that means either produced a cut or was actually best

        Boolean tt_hash_collisions{};
        Boolean tt_had_move_cached{};
        Boolean tt_move_is_exact_match{};
        Boolean tt_move_makes_cut{};
        Boolean tt_move_is_best{};

        Boolean killer_move_exists{};
        Boolean killer_move_is_legal{};
        Boolean killer_move_makes_cut{};
        Boolean killer_move_is_best{};

        Boolean first_move_makes_cut{};
        Boolean first_move_is_best{};

        Boolean negascout_re_search{};

        Number num_moves_considered{};

        auto to_string() const -> std::string
        {
            return "--- Diagnostics ---\n" +
                fmt::format("tt_hash_collisions: {}\n", tt_hash_collisions.to_string()) +
                fmt::format("tt_had_move_cached: {}\n", tt_had_move_cached.to_string()) +
                fmt::format("tt_move_is_exact_match: {}\n", tt_move_is_exact_match.to_string()) +
                fmt::format("tt_move_makes_cut: {}\n", tt_move_makes_cut.to_string()) +
                fmt::format("tt_move_is_best: {}\n", tt_move_is_best.to_string()) +
                fmt::format("killer_move_exists: {}\n", killer_move_exists.to_string()) +
                fmt::format("killer_move_is_legal: {}\n", killer_move_is_legal.to_string()) +
                fmt::format("killer_move_makes_cut: {}\n", killer_move_makes_cut.to_string()) +
                fmt::format("killer_move_is_best: {}\n", killer_move_is_best.to_string()) +
                fmt::format("first_move_makes_cut: {}\n", first_move_makes_cut.to_string()) +
                fmt::format("first_move_is_best: {}\n", first_move_is_best.to_string()) +
                fmt::format("negascout_re_search: {}\n", negascout_re_search.to_string()) +
                fmt::format("num_moves_considered: {}\n", num_moves_considered.to_string());
        }
    };

    Diagnostics diagnostics{};
#endif

#ifdef USE_TT
#ifdef USE_CUSTOM_TT
    auto compute_hash(u64 friends, u64 enemies) -> std::size_t
    {
        auto const key = std::tuple{friends, enemies};
        return absl::Hash<decltype(key)>{}(key);
    }

    struct TranspositionTable
    {
    public:
        struct Value
        {
        private:
            friend struct TranspositionTable;

            u64 friends_{};
            u64 enemies_{};

            constexpr auto matches(u64 f, u64 e) const -> bool
            {
                return f == friends_ && e == enemies_;
            }

        public:
            constexpr auto set_key(u64 friends, u64 enemies) -> void
            {
                friends_ = friends;
                enemies_ = enemies;
            }

            InternalMoveRecommendation recommendation{};
            int depth{};
            NodeType type{};
        };

        static constexpr auto size = std::size_t{20};

        TranspositionTable() : data_(std::size_t{2} << size, Value{}) {}

        auto reset() -> void { std::fill(data_.begin(), data_.end(), Value{}); }

        struct LookupResult
        {
            Value* value;
            bool was_found;
        };

        auto lookup(u64 friends, u64 enemies) -> LookupResult
        {
            auto const index = compute_hash(friends, enemies) % (std::size_t{2} << size);
            auto* value = &data_[index];
#ifdef DIAGNOSTICS
            bool const is_collision = (value->friends_ != 0 || value->enemies_ != 0) &&
                !value->matches(friends, enemies);
            diagnostics.tt_hash_collisions.update(is_collision);
#endif
            return {value, value->matches(friends, enemies)};
        }

    private:
        std::vector<Value> data_{};
    };
#else
    struct TranspositionTable
    {
    public:
        using Key = std::pair<u64, u64>;

        struct Value
        {
            constexpr auto set_key(u64, u64) -> void {}

            InternalMoveRecommendation recommendation{};
            int depth{};
            NodeType type{};
        };

        TranspositionTable() = default;

        auto reset() -> void { data_ = {}; }

        struct LookupResult
        {
            Value* value;
            bool was_found;
        };

        auto lookup(u64 friends, u64 enemies) -> LookupResult
        {
            auto const [it, did_insert] = data_.try_emplace(std::pair{friends, enemies});
            return {&it->second, !did_insert};
        }

    private:
        absl::flat_hash_map<Key, Value> data_{};
    };
#endif

    TranspositionTable table{};
    int pv_count{};
#endif

    struct Searcher
    {
#ifdef USE_KH
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
#ifdef USE_TT
        auto add_to_transposition_table() -> void;
#endif

        // Input arguments
        BitBoard friends_;
        BitBoard enemies_;
        int depth_;
        ScoreType alpha_;
        ScoreType beta_;
#ifdef USE_KH
        InternalMove killer_move_;
#endif

        // Internal data
#ifdef USE_KH
        InternalMove next_killer_move_{};
#endif
        InternalMoveRecommendation best_result_;
        NodeType node_type_;
        int move_count_{};

#ifdef DIAGNOSTICS
        Diagnostics::Scratchpad scratchpad_{};
#endif
    };

#ifdef USE_KH
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
#ifdef USE_KH
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
            add_to_transposition_table();

#ifdef DIAGNOSTICS
            if (scratchpad_.first_move_score)
            {
                diagnostics.first_move_is_best.update(
                    *scratchpad_.first_move_score >= best_result_.score);
            }
            if (scratchpad_.killer_move_score)
            {
                diagnostics.killer_move_is_best.update(
                    *scratchpad_.killer_move_score >= best_result_.score);
            }
            if (scratchpad_.tt_move_score)
            {
                diagnostics.tt_move_is_best.update(
                    *scratchpad_.tt_move_score >= best_result_.score);
            }
            diagnostics.num_moves_considered.update(move_count_);
#endif

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

#ifdef USE_NS
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

#ifdef DIAGNOSTICS
            diagnostics.negascout_re_search.update(must_re_search);
#endif
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
#ifdef USE_KH
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

#ifdef DIAGNOSTICS
        if (move_count_ == 0)
        {
            diagnostics.first_move_makes_cut.update(node_type_ == NodeType::Cut);
            scratchpad_.first_move_score = score;
        }
        if (scratchpad_.processing_tt_move)
        {
            diagnostics.tt_move_makes_cut.update(node_type_==NodeType::Cut);
            scratchpad_.tt_move_score = score;
            scratchpad_.processing_tt_move = false;
        }
        if (scratchpad_.processing_killer_move)
        {
            diagnostics.killer_move_makes_cut.update(node_type_==NodeType::Cut);
            scratchpad_.killer_move_score = score;
            scratchpad_.processing_killer_move = false;
        }
#endif

        ++move_count_;
    }

    auto Searcher::main_search() -> void
    {
#ifdef USE_KH
#ifdef DIAGNOSTICS
        diagnostics.killer_move_exists.update(!killer_move_.empty());
        if (!killer_move_.empty())
        {
            diagnostics.killer_move_is_legal.update(
                is_move_legal(killer_move_, friends_, enemies_));
        }
#endif
#endif

        best_result_ = InternalMoveRecommendation{InternalMove{}, -big};
        node_type_ = NodeType::All;

#ifdef USE_TT
        // Check the transposition table before checking if the game is over
        // (this works out faster)
        auto const [tt_ptr, was_found] = table.lookup(friends_, enemies_);
        auto tt_move = InternalMove{};
#ifdef DIAGNOSTICS
        diagnostics.tt_had_move_cached.update(was_found);
#endif
        if (was_found)
        {
            // Note: don't just return a previous result, even one from a higher
            // depth!
            //
            // The beta cutoff might have been lower for this version of the
            // result, which means the score might not be enough to trigger a
            // parent's beta cutoff where it may be if we let the search
            // continue.
            //
            // I'm not sure about this analysis, but certainly I saw
            // performance degradation if I just returned from here when I
            // found a match from a higher depth search.
            //
            // If the node was a pv node however, it is safe to return straight
            // away. This doesn't seem to produce much advantage, but might make
            // a difference in some cases.
            tt_move = tt_ptr->recommendation.move;

            bool const tt_is_exact_match =
                (tt_ptr->type == NodeType::Pv && tt_ptr->depth >= depth_) || (tt_move.empty());
#ifdef DIAGNOSTICS
            diagnostics.tt_move_is_exact_match.update(tt_is_exact_match);
#endif
            if (tt_is_exact_match)
            {
                best_result_ = tt_ptr->recommendation;
                return;
            }

#ifdef DIAGNOSTICS
            scratchpad_.processing_tt_move = true;
#endif
            this->process_move(tt_move);
            if (node_type_ == NodeType::Cut)
                return;
        }
#endif

#ifdef USE_KH
        if (!killer_move_.empty() && is_move_legal(killer_move_, friends_, enemies_))
        {
#ifdef DIAGNOSTICS
            scratchpad_.processing_killer_move = true;
#endif
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

#ifdef USE_KH
                if (killer_move_ == move)
                    continue;
#endif

#ifdef USE_TT
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

#ifdef USE_TT
    auto Searcher::add_to_transposition_table() -> void
    {
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
    }
#endif

}  // namespace

auto recommend_move(Position const& position) -> MoveRecommendation
{
#ifdef USE_TT
    table.reset();
    pv_count = {};
#endif
#ifdef DIAGNOSTICS
    diagnostics = {};
#endif
    auto const friends = position.board()[position.player_to_move()];
    auto const enemies = position.board()[!position.player_to_move()];

    auto internal = InternalMoveRecommendation{};

#ifdef USE_ID
    for (auto depth = 1; depth <= DEPTH; ++depth)
#else
    auto const depth = int{DEPTH};
#endif
    {
#ifdef USE_NEW
        internal = Searcher(friends, enemies, depth, -big, big).search();
#else
        internal = recommend_move_search(friends, enemies, depth, -big, big);
#endif
    }

#ifdef USE_TT

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
