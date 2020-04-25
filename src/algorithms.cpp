#include "rock/algorithms.h"
#include "bit_operations.h"
#include "rock/parse.h"
#include "table_generation.h"
#include <algorithm>

// TODO:
// - Consider using strong types more, instead of lots of u64s
//   - Reconsider integer type used to store board position if this is done
//     - No longer needs to be compact (may perform better if not)
// - Should probably work out some performance regression testing
// - Consider better algorithm for detemining if game is over
//   - This check is probably done too many times in the search code
// - Create some kind of stateful AI object to remember information

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

        constexpr auto empty() const -> bool { return from_board == u64{} && to_board == u64{}; }

        auto to_standard_move() const -> Move
        {
            return Move{
                BoardCoordinates{coordinates_from_bit_board(from_board)},
                BoardCoordinates{coordinates_from_bit_board(to_board)},
            };
        }
    };

    struct InternalMoveRecommendation
    {
        InternalMove move;
        double score;

        auto to_standard_move_recommendation() const -> MoveRecommendation
        {
            return {
                move.to_standard_move(),
                score,
            };
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
    constexpr double big = 10000.0;

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
    constexpr std::pair<BitBoard, double> important_positions[] = {
        {all_circles.data[BoardCoordinates{3, 3}.data()][3], 1.0},
        {all_circles.data[BoardCoordinates{3, 3}.data()][2], 1.0},
        {all_circles.data[BoardCoordinates{3, 3}.data()][1], 1.0},
    };

    auto evaluate_leaf_position(BitBoard friends, BitBoard enemies) -> double
    {
        auto res = double{};

        bool const has_player_won = are_pieces_all_together(friends);
        bool const has_player_lost = are_pieces_all_together(enemies);

        if (has_player_lost || has_player_won)
            res += 1000.0 * static_cast<double>(has_player_won - has_player_lost);

        for (auto const& [positions, value] : important_positions)
        {
            res += value * static_cast<double>(pop_count(positions & friends));
            res -= value * static_cast<double>(pop_count(positions & enemies));
        }

        return res;
    }

    auto is_game_finished(BitBoard friends, BitBoard enemies) -> bool
    {
        return count_moves(friends, enemies, 1) == 0 || are_pieces_all_together(friends) ||
            are_pieces_all_together(enemies);
    }

    /**
     * Simple negamax algorithm presented here for clarity
     */
    [[maybe_unused]] auto
    recommend_move_negamax(BitBoard const friends, BitBoard const enemies, int const depth)
        -> InternalMoveRecommendation
    {
        if (depth == 0 || is_game_finished(friends, enemies))
            return {{}, evaluate_leaf_position(friends, enemies)};

        auto result = InternalMoveRecommendation{{}, -big};

        auto moves = generate_moves(friends, enemies);
        for (auto move_set : moves)
        {
            while (move_set.to_board)
            {
                auto const to_board = extract_one_bit(move_set.to_board);

                auto friends_copy = friends;
                auto enemies_copy = enemies;
                apply_move_low_level(move_set.from_board, to_board, &friends_copy, &enemies_copy);

                auto const recommendation =
                    recommend_move_negamax(enemies_copy, friends_copy, depth - 1);
                auto const score = -recommendation.score;

                if (score > result.score)
                {
                    result.move = {move_set.from_board, to_board};
                    result.score = score;
                }
            }
        }

        return result;
    }

    /**
     * Negamax algorithm with alpha-beta pruning and the killer heuristic.
     * The `killer_move` parameter may be empty.
     */
    auto recommend_move_negamax_ab_killer(
        BitBoard const friends,
        BitBoard const enemies,
        int depth,
        double alpha,
        double beta,
        InternalMove killer_move) -> InternalMoveRecommendation
    {
        if (depth == 0 || is_game_finished(friends, enemies))
            return {{}, evaluate_leaf_position(friends, enemies)};

        auto result = InternalMoveRecommendation{{}, -big};

        auto moves = generate_moves(friends, enemies);

        if (!killer_move.empty())
        {
            auto const move_matches_killer = [&killer_move](InternalMove const& move) {
                return (move.from_board & killer_move.from_board) &&
                    (move.to_board & killer_move.to_board);
            };

            if (auto it = std::find_if(moves.begin(), moves.end(), move_matches_killer);
                it != moves.end())
            {
                std::swap(*moves.begin(), *it);
            }
            killer_move = {};
        }

        for (auto move_set : moves)
        {
            while (move_set.to_board)
            {
                auto const to_board = extract_one_bit(move_set.to_board);

                auto friends_copy = friends;
                auto enemies_copy = enemies;
                apply_move_low_level(move_set.from_board, to_board, &friends_copy, &enemies_copy);

                auto const recommendation = recommend_move_negamax_ab_killer(
                    enemies_copy, friends_copy, depth - 1, -beta, -alpha, killer_move);
                auto const score = -recommendation.score;

                if (score > result.score)
                {
                    result.move = {move_set.from_board, to_board};
                    result.score = score;
                    killer_move = recommendation.move;
                }

                if (result.score > alpha)
                {
                    // Until this happens, we are an 'All-Node'
                    // Now we may be a 'PV-Node', or...
                    alpha = result.score;
                }

                if (alpha >= beta)
                {
                    // ...if this happens, we are a 'Cut-Node'
                    goto end;
                }
            }
        }

    end:

        // Note, we may return values outside of the range [alpha, beta]. This
        // makes us a 'fail-soft' version of alpha-beta pruning
        return result;
    }
}  // namespace

auto recommend_move(Position const& position) -> MoveRecommendation
{
    InternalMoveRecommendation const internal = recommend_move_negamax_ab_killer(
        position.board()[position.player_to_move()],
        position.board()[!position.player_to_move()],
        6,
        -big,
        big,
        {});

    return internal.to_standard_move_recommendation();
}

auto evaluate_position(Position const& position) -> double
{
    return recommend_move(position).score;
}

auto normalize_score(double const score, Player const player) -> double
{
    return player == Player::Black ? -score : score;
}

}  // namespace rock
