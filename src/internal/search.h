#pragma once

#include "evaluate.h"
#include "internal_types.h"
#include "transposition_table.h"

namespace rock::internal
{

struct Searcher
{
    explicit Searcher(
        BitBoard friends,
        BitBoard enemies,
        int depth,
        ScoreType alpha,
        ScoreType beta,
        TranspositionTable* table,
        InternalMove killer_move = {})
        : friends_{friends},
          enemies_{enemies},
          depth_{depth},
          alpha_{alpha},
          beta_{beta},
          table_{table},
          killer_move_{killer_move}
    {}

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
    TranspositionTable* table_;
    InternalMove killer_move_;

    // Internal data
    InternalMove next_killer_move_{};
    InternalMoveRecommendation best_result_;
    NodeType node_type_;
    u64 move_count_{};

#ifdef DIAGNOSTICS
    Diagnostics::Scratchpad scratchpad_{};
#endif
};

inline auto Searcher::search_next(BitBoard friends, BitBoard enemies, ScoreType alpha, ScoreType beta)
    -> InternalMoveRecommendation
{
    auto searcher = Searcher(friends, enemies, depth_ - 1, alpha, beta, table_, next_killer_move_);
    return searcher.search();
}

inline auto Searcher::search() -> InternalMoveRecommendation
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

inline auto Searcher::process_move(InternalMove move) -> void
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
        next_killer_move_ = recommendation.move;
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

inline auto Searcher::main_search() -> void
{
    DIAGNOSTICS_UPDATE_BEFORE_SEARCH();

    best_result_ = InternalMoveRecommendation{InternalMove{}, -big};
    node_type_ = NodeType::All;

    // Check the transposition table before checking if the game is over
    // (this works out faster)
    auto const [tt_ptr, was_found] = table_->lookup(friends_, enemies_);
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

    if (!killer_move_.empty() && is_move_legal(killer_move_, friends_, enemies_))
    {
        DIAGNOSTICS_PREPARE_KILLER_MOVE();
        this->process_move(killer_move_);
        if (node_type_ == NodeType::Cut)
            return;
    }

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

            if (killer_move_ == move)
                continue;

            if (tt_move == move)
                continue;

            this->process_move(move);
            if (node_type_ == NodeType::Cut)
                return;
        }
    }

    // Note, we may return values outside of the range [alpha, beta] (if we
    // are an 'all' node and score below alpha). This makes us a 'fail-soft'
    // version of alpha-beta pruning.
}

inline auto Searcher::add_to_transposition_table() -> void
{
    auto const [tt_ptr, was_found] = table_->lookup(friends_, enemies_);

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

}  // namespace rock::internal
