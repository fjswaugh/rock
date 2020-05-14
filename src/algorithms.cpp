//#define NO_USE_NEGASCOUT
//#define DIAGNOSTICS

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

auto analyze_position(Position const& position, int max_depth) -> PositionAnalysis
{
    static auto table = TranspositionTable(19);

    table.reset();

    auto recommendation = InternalMoveRecommendation{};
    for (auto depth = 1; depth <= max_depth; ++depth)
    {
        auto searcher = Searcher(depth, &table);
        recommendation = searcher.search(position.friends(), position.enemies());
    }
    return make_analysis(position, recommendation, table);
}

auto analyze_available_moves(Position const& position, int max_depth)
    -> std::map<Move, PositionAnalysis>
{
    auto result = std::map<Move, PositionAnalysis>{};

    for (auto const move : list_moves(position))
    {
        auto const new_position = apply_move(move, position);
        result[move] = analyze_position(new_position, max_depth - 1);
    }

    return result;
}

auto select_analysis_with_softmax(
    std::map<Move, PositionAnalysis> const& moves, double softmax_parameter) -> PositionAnalysis
{
    static auto rng = std::ranlux24{std::random_device{}()};

    // Weight each move according to its score and the softmax function
    auto weights = std::vector<double>(moves.size());
    auto get_weight = [&](std::pair<Move, PositionAnalysis> const& move) {
        return std::exp(softmax_parameter * 0.1 * move.second.score);
    };
    std::transform(moves.begin(), moves.end(), weights.begin(), get_weight);

    // Pick an index based on the weight
    auto dist = std::discrete_distribution<std::size_t>(weights.begin(), weights.end());
    auto const index = dist(rng);

    // Select the move
    auto it = std::next(moves.begin(), static_cast<std::ptrdiff_t>(index));

    // And construct the analysis
    auto [selected_move, analysis] = *it;
    analysis.best_move = selected_move;
    analysis.principal_variation.insert(analysis.principal_variation.begin(), selected_move);
    return analysis;
}

namespace
{
    auto depth_from_difficulty(int difficulty) -> int
    {
        assert(difficulty >= 0);
        switch (difficulty)
        {
        case 0:
        case 1:
            return 1;
        case 2:
            return 2;
        case 3:
        case 4:
        case 5:
            return 3;
        case 6:
        case 7:
            return 4;
        case 8:
            return 5;
        case 9:
            return 6;
        default:
            return 6 + difficulty - 10;
        }
    }

    auto softmax_parameter_from_difficulty(int difficulty) -> std::optional<double>
    {
        assert(difficulty >= 0);
        switch (difficulty)
        {
        case 0:
            return 0.0;
        case 1:
            return 0.2;
        case 2:
            return 0.4;
        case 3:
            return 0.6;
        case 4:
            return 0.8;
        case 5:
            return 1.0;
        case 6:
            return 1.5;
        case 7:
            return 3.0;
        case 8:
            return 4.5;
        case 9:
            return 8.0;
        default:
            return std::nullopt;
        }
    }
}  // namespace

auto analyze_position_with_ai_difficulty_level(Position const& position, int ai_level)
    -> PositionAnalysis
{
    auto const depth = depth_from_difficulty(ai_level);
    auto const softmax = softmax_parameter_from_difficulty(ai_level);

    if (softmax.has_value())
    {
        auto const moves = analyze_available_moves(position, depth);
        return select_analysis_with_softmax(moves, softmax.value());
    }
    else
    {
        return analyze_position(position, depth);
    }
}

struct GameAnalyzer::Impl
{
    Impl() = default;

    bool is_analyzing{};
    TranspositionTable transposition_table{};
    Position position{};
    InternalMoveRecommendation best_recommendation_so_far{};
    int current_depth{};
    int max_depth{100};
    std::function<void(GameAnalyzer&)> report_callback{};
    bool stop_requested{};
};

GameAnalyzer::GameAnalyzer() : impl_{std::make_unique<Impl>()}
{}

auto GameAnalyzer::analyze_position(Position position) -> void
{
    if (impl_->is_analyzing)
        return;

    impl_->stop_requested = false;
    impl_->is_analyzing = true;
    impl_->transposition_table.reset();
    impl_->best_recommendation_so_far = InternalMoveRecommendation{};
    impl_->current_depth = 0;
    impl_->position = position;

    for (; impl_->current_depth <= impl_->max_depth; ++impl_->current_depth)
    {
        auto searcher =
            Searcher(impl_->current_depth, &impl_->transposition_table, &impl_->stop_requested);

        auto const recommendation =
            searcher.search(impl_->position.friends(), impl_->position.enemies());

        if (impl_->stop_requested)
        {
            // Recommendation is incomplete
            if (recommendation.score > impl_->best_recommendation_so_far.score)
                impl_->best_recommendation_so_far = recommendation;
            break;
        }

        impl_->best_recommendation_so_far = recommendation;

        if (impl_->report_callback)
            impl_->report_callback(*this);
    }

    impl_->is_analyzing = false;
}

auto GameAnalyzer::stop_analysis() -> void
{
    impl_->stop_requested = true;
}

auto GameAnalyzer::is_analysis_ongoing() const -> bool
{
    return impl_->is_analyzing;
}

auto GameAnalyzer::set_max_depth(int max) -> void
{
    impl_->max_depth = max;
}

auto GameAnalyzer::set_report_callback(std::function<void(GameAnalyzer&)> f) -> void
{
    impl_->report_callback = std::move(f);
}

auto GameAnalyzer::best_analysis_so_far() -> PositionAnalysis
{
    return make_analysis(
        impl_->position, impl_->best_recommendation_so_far, impl_->transposition_table);
}

auto GameAnalyzer::current_depth() const -> int
{
    return impl_->current_depth;
}

GameAnalyzer::~GameAnalyzer() = default;

}  // namespace rock
