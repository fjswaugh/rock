#include "internal_types.h"
#include "transposition_table.h"

namespace rock::internal
{

namespace
{
    auto extract_pv_line(Position p, TranspositionTable const& table) -> std::vector<Move>
    {
        auto moves = std::vector<Move>{};

        while (true)
        {
            auto const [value, was_found] = table.lookup(p.friends(), p.enemies());
            if (!was_found || value->type != NodeType::Pv)
                break;

            auto const move = value->recommendation.move.to_standard_move();

            if (!move.has_value())
                break;
            moves.push_back(*move);

            p = apply_move(*move, p);
        }

        return moves;
    }

    auto normalize_score(ScoreType score, Player player) -> ScoreType
    {
        return player == Player::Black ? -score : score;
    }
}  // namespace

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
