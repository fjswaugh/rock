#include "rock/algorithms.h"
#include "rock/parse.h"
#include "table_generation.h"
#include <algorithm>
#include <climits>

// TODO:
// Consider a little more consistency in naming of internal functions
// - Consider moving away from upfront generation of moves
//   - Perhaps use code like in list_legal_destinations()
//   - Should be faster than separating into individual move objects anyway
//   - Reconsider integer type used to store board position if this is done
//     - No longer needs to be compact (may perform better if not)
// - Remove duplication in list_legal_destinations() and generate_moves()
// - Separate bitwise stuff into separate header
// - Remove separate search algorithms
//   - Should probably work out some performance regression testing
// - Consider better algorithm for detemining if game is over
//   - This check is probably done too many times in the search code
// - Create some kind of stateful AI object to remember information

namespace rock
{

namespace
{
    auto apply_move_low_level(Move const m, BitBoard* mine, BitBoard* theirs) -> void
    {
        auto const from = m.from.bit_board();
        auto const to = m.to.bit_board();

        mine->data ^= (from | to);
        theirs->data &= ~to;
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

namespace
{
    struct MoveList
    {
        constexpr void push_back(Move move)
        {
            assert(size_ < max_size);
            moves_[size_++] = move;
        }
        constexpr auto begin() const -> Move const* { return &moves_[0]; }
        constexpr auto end() const -> Move const* { return this->begin() + size_; }
        constexpr auto begin() -> Move* { return &moves_[0]; }
        constexpr auto end() -> Move* { return this->begin() + size_; }
        constexpr auto size() const { return size_; }

    private:
        constexpr static auto max_size = 12 * 8;

        Move moves_[max_size];
        std::size_t size_{};
    };

    constexpr double big = 1000.0;

    template <typename T, std::size_t N>
    using array_ref = T const (&)[N];

    constexpr auto all_circles = make_all_circles();
    constexpr auto all_directions = make_all_directions();

    static_assert(CHAR_BIT == 8);

    template <typename T>
    [[maybe_unused]] constexpr auto num_bits = sizeof(T) * 8;

    [[maybe_unused]] inline auto pop_count_manual(u64 x) -> u64
    {
        // clang-format off
        static constexpr int const s[] = {1, 2, 4, 8, 16, 32};
        static constexpr u64 const b[] = {
            0x5555555555555555,
            0x3333333333333333,
            0x0F0F0F0F0F0F0F0F,
            0x00FF00FF00FF00FF,
            0x0000FFFF0000FFFF,
            0x00000000FFFFFFFF,
        };

        u64 c;
        c = x - ((x >> u64{1}) & b[0]);
        c = ((c >> s[1]) & b[1]) + (c & b[1]);
        c = ((c >> s[2]) + c) & b[2];
        c = ((c >> s[3]) + c) & b[3];
        c = ((c >> s[4]) + c) & b[4];
        c = ((c >> s[5]) + c) & b[5];
        return c;
        // clang-format on
    }

    [[maybe_unused]] inline auto position_from_bit_board_manual(u64 x) -> u64
    {
        // clang-format off
        auto c = u64{64};
        x &= static_cast<u64>(-static_cast<std::int64_t>(x));
        if (x) c--;
        if (x & u64{0x00000000FFFFFFFF}) c -= 32;
        if (x & u64{0x0000FFFF0000FFFF}) c -= 16;
        if (x & u64{0x00FF00FF00FF00FF}) c -=  8;
        if (x & u64{0x0F0F0F0F0F0F0F0F}) c -=  4;
        if (x & u64{0x3333333333333333}) c -=  2;
        if (x & u64{0x5555555555555555}) c -=  1;
        return c;
        // clang-format on
    }

    inline auto pop_count(u64 x) -> u64
    {
#if defined(__GNUC__)
        if constexpr (num_bits<unsigned long> == 64)
        {
            return static_cast<u64>(__builtin_popcountl(x));
        }
        else
        {
            static_assert(num_bits<unsigned long long> == 64);
            return static_cast<u64>(__builtin_popcountll(x));
        }
#else
        return pop_count_manual(x);
#endif
    }

    inline auto position_from_bit_board(u64 x) -> u64
    {
#if defined(__GNUC__)
        if constexpr (num_bits<unsigned long> == 64)
        {
            return static_cast<u64>(__builtin_ctzl(x));
        }
        else
        {
            static_assert(num_bits<unsigned long long> == 64);
            return static_cast<u64>(__builtin_ctzll(x));
        }
#else
        return position_from_bit_board_manual(x);
#endif
    }

    inline auto bit_board_from_position(u64 pos) -> u64 { return u64{1} << pos; }

    auto generate_legal_destinations_impl(
        BoardPosition const from, BitBoard const friends, BitBoard const enemies) -> u64
    {
        auto result = u64{};
        auto const all_pieces = friends | enemies;

        auto const positive = (~u64{}) << static_cast<u64>(from.data());
        auto const negative = ~positive;

        array_ref<u64, 4> directions = all_directions.data[from.data()];
        array_ref<u64, 8> circles = all_circles.data[from.data()];

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

    auto generate_legal_destinations(BoardPosition const from, Position const& position) -> u64
    {
        auto const friends = position.board()[position.player_to_move()];
        auto const enemies = position.board()[!position.player_to_move()];
        return generate_legal_destinations_impl(from, friends, enemies);
    }

    auto generate_moves_impl(BitBoard const friends, BitBoard const enemies) -> MoveList
    {
        auto const all_pieces = enemies | friends;

        auto list = MoveList{};

        auto pieces_to_process = friends;
        while (pieces_to_process)
        {
            auto const pos = position_from_bit_board(pieces_to_process);
            pieces_to_process ^= bit_board_from_position(pos);

            auto const positive = (~u64{}) << u64(pos);
            auto const negative = ~positive;

            array_ref<u64, 4> directions = all_directions.data[pos];
            array_ref<u64, 8> circles = all_circles.data[pos];

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
                {
                    auto const to = position_from_bit_board(ce_d_p);
                    list.push_back({BoardPosition{pos}, BoardPosition{to}});
                }

                if (ce_d_n && (enemies & c_d_n) == u64{} && (friends & ce_d_n) == u64{})
                {
                    auto const to = position_from_bit_board(ce_d_n);
                    list.push_back({BoardPosition{pos}, BoardPosition{to}});
                }
            }
        }

        return list;
    }

    auto count_moves_impl(BitBoard const friends, BitBoard const enemies, int level) -> std::size_t
    {
        if (level <= 0)
            return 1;

        auto moves = generate_moves_impl(friends, enemies);

        if (level == 1)
            return moves.size();

        auto num_moves = std::size_t{};

        for (auto const move : moves)
        {
            auto enemies_copy = enemies;
            auto friends_copy = friends;
            apply_move_low_level(move, &friends_copy, &enemies_copy);

            num_moves += count_moves_impl(enemies_copy, friends_copy, level - 1);
        }

        return num_moves;
    }

    auto generate_moves(Position const& position) -> MoveList
    {
        auto const friends = position.board()[position.player_to_move()];
        auto const enemies = position.board()[!position.player_to_move()];
        return generate_moves_impl(friends, enemies);
    }
}  // namespace

auto list_moves(Position const& position) -> std::vector<Move>
{
    auto const moves = generate_moves(position);
    return std::vector(moves.begin(), moves.end());
}

auto count_moves(Position const& position, int level) -> std::size_t
{
    return count_moves_impl(
        position.board()[position.player_to_move()],
        position.board()[!position.player_to_move()],
        level);
}

auto is_legal_move(Move move, Position const& position) -> bool
{
    auto const all_moves = generate_moves(position);
    return std::find(all_moves.begin(), all_moves.end(), move) != all_moves.end();
}

auto list_legal_destinations(BoardPosition from, Position const& position)
    -> std::vector<BoardPosition>
{
    auto destinations = generate_legal_destinations(from, position);

    auto res = std::vector<BoardPosition>{};
    res.reserve(8);

    while (destinations)
    {
        auto const board_pos = position_from_bit_board(destinations);
        res.emplace_back(board_pos);
        destinations ^= bit_board_from_position(board_pos);
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
            auto const pos = position_from_bit_board(pieces);
            auto const pos_board = bit_board_from_position(pos);

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
    auto const pos = position_from_bit_board(board);
    auto const pos_board = bit_board_from_position(pos);

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

namespace
{
    constexpr std::pair<BitBoard, double> important_positions[] = {
        {all_circles.data[BoardPosition{3, 3}.data()][3], 1.0},
        {all_circles.data[BoardPosition{3, 3}.data()][2], 1.0},
        {all_circles.data[BoardPosition{3, 3}.data()][1], 1.0},
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
        return count_moves_impl(friends, enemies, 1) == 0 || are_pieces_all_together(friends) ||
            are_pieces_all_together(enemies);
    }

    [[maybe_unused]] auto
    recommend_move_negamax(BitBoard const friends, BitBoard const enemies, int const depth)
        -> MoveRecommendation
    {
        if (depth == 0 || is_game_finished(friends, enemies))
            return {{}, evaluate_leaf_position(friends, enemies)};

        auto best_score = -big;
        auto best_move = Move{};

        for (auto move : generate_moves_impl(friends, enemies))
        {
            auto friends_copy = friends;
            auto enemies_copy = enemies;
            apply_move_low_level(move, &friends_copy, &enemies_copy);

            auto const recommendation =
                recommend_move_negamax(enemies_copy, friends_copy, depth - 1);
            auto const score = -recommendation.score;

            if (score > best_score)
            {
                best_score = score;
                best_move = move;
            }
        }

        return {best_move, best_score};
    }

    [[maybe_unused]] auto recommend_move_negamax_ab(
        BitBoard const friends, BitBoard const enemies, int depth, double alpha, double beta)
        -> MoveRecommendation
    {
        if (depth == 0 || is_game_finished(friends, enemies))
            return {{}, evaluate_leaf_position(friends, enemies)};

        auto best_score = -big;
        auto best_move = Move{};

        for (auto move : generate_moves_impl(friends, enemies))
        {
            auto friends_copy = friends;
            auto enemies_copy = enemies;
            apply_move_low_level(move, &friends_copy, &enemies_copy);

            auto const recommendation =
                recommend_move_negamax_ab(enemies_copy, friends_copy, depth - 1, -beta, -alpha);
            auto const score = -recommendation.score;

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
        BitBoard const friends,
        BitBoard const enemies,
        int depth,
        double alpha,
        double beta,
        std::optional<Move> killer_move) -> MoveRecommendation
    {
        if (depth == 0 || is_game_finished(friends, enemies))
            return {{}, evaluate_leaf_position(friends, enemies)};

        auto best_score = -big;
        auto best_move = Move{};

        auto all_moves = generate_moves_impl(friends, enemies);

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
            auto friends_copy = friends;
            auto enemies_copy = enemies;
            apply_move_low_level(move, &friends_copy, &enemies_copy);

            auto const recommendation = recommend_move_negamax_ab_killer(
                enemies_copy, friends_copy, depth - 1, -beta, -alpha, killer_move);
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

auto recommend_move(Position const& position) -> MoveRecommendation
{
    return recommend_move_negamax_ab_killer(
        position.board()[position.player_to_move()],
        position.board()[!position.player_to_move()],
        6,
        -big,
        big,
        {});
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
