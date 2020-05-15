#include "internal_types.h"
#include "transposition_table.h"

namespace rock::internal
{

static auto extract_pv_line(Position p, TranspositionTable const& table) -> std::vector<Move>
{
    auto already_seen_positions = std::vector<Position>{};
    auto moves = std::vector<Move>{};

    while (true)
    {
        // Exit early if we encounter the same position twice
        if (std::find(already_seen_positions.begin(), already_seen_positions.end(), p) !=
            already_seen_positions.end())
        {
            break;
        }

        auto const [value, was_found] = table.lookup(p.friends(), p.enemies());
        if (!was_found || value->type != NodeType::Pv)
            break;

        std::optional<Move> const recommended_move = value->recommendation.move.to_standard_move();
        if (!recommended_move.has_value())
            break;

        moves.push_back(*recommended_move);
        already_seen_positions.push_back(p);

        p = apply_move(*recommended_move, p);
    }

    return moves;
}

static auto normalize_score(ScoreType score, Player player) -> ScoreType
{
    return player == Player::Black ? -score : score;
}

auto make_analysis(Position const& p, InternalMoveRecommendation const& r) -> PositionAnalysis
{
    auto result = PositionAnalysis{};

    result.score = normalize_score(r.score, p.player_to_move());
    result.best_move = r.move.to_standard_move();

    return result;
}

auto make_analysis(
    Position const& p, InternalMoveRecommendation const& r, TranspositionTable const& t)
    -> PositionAnalysis
{
    auto result = make_analysis(p, r);

    result.principal_variation = extract_pv_line(p, t);

    return result;
}

}  // namespace rock::internal
