#pragma once

#include "rock/common.h"
#include <fmt/format.h>

namespace rock::internal
{

#ifndef DIAGNOSTICS
#define DIAGNOSTICS_UPDATE(FIELD, VALUE)
#define DIAGNOSTICS_PREPARE_KILLER_MOVE()
#define DIAGNOSTICS_PREPARE_TT_MOVE()
#define DIAGNOSTICS_UPDATE_BEFORE_SEARCH()
#define DIAGNOSTICS_UPDATE_AFTER_SEARCH(BEST_RESULT, MOVE_COUNT)
#define DIAGNOSTICS_UPDATE_AFTER_MOVE(TYPE, SCORE)
#endif

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

#define DIAGNOSTICS_UPDATE(FIELD, VALUE) diagnostics.FIELD.update(VALUE)
#define DIAGNOSTICS_PREPARE_KILLER_MOVE() scratchpad_.processing_killer_move = true
#define DIAGNOSTICS_PREPARE_TT_MOVE() scratchpad_.processing_tt_move = true

#ifndef NO_USE_KILLER
#define DIAGNOSTICS_UPDATE_BEFORE_SEARCH()                                                         \
    do                                                                                             \
    {                                                                                              \
        diagnostics.killer_move_exists.update(!killer_move_.empty());                              \
        if (!killer_move_.empty())                                                                 \
            diagnostics.killer_move_is_legal.update(                                               \
                is_move_legal(killer_move_, friends_, enemies_));                                  \
    } while (false)
#else
#define DIAGNOSTICS_UPDATE_BEFORE_SEARCH()
#endif

#define DIAGNOSTICS_UPDATE_AFTER_SEARCH(BEST_RESULT, MOVE_COUNT)                                   \
    do                                                                                             \
    {                                                                                              \
        if (scratchpad_.first_move_score)                                                          \
        {                                                                                          \
            diagnostics.first_move_is_best.update(                                                 \
                *scratchpad_.first_move_score >= BEST_RESULT.score);                               \
        }                                                                                          \
        if (scratchpad_.killer_move_score)                                                         \
        {                                                                                          \
            diagnostics.killer_move_is_best.update(                                                \
                *scratchpad_.killer_move_score >= BEST_RESULT.score);                              \
        }                                                                                          \
        if (scratchpad_.tt_move_score)                                                             \
        {                                                                                          \
            diagnostics.tt_move_is_best.update(*scratchpad_.tt_move_score >= BEST_RESULT.score);   \
        }                                                                                          \
        diagnostics.num_moves_considered.update(static_cast<u64>(MOVE_COUNT));                     \
    } while (false)

#define DIAGNOSTICS_UPDATE_AFTER_MOVE(TYPE, SCORE)                                                 \
    do                                                                                             \
    {                                                                                              \
        if (move_count_ == 0)                                                                      \
        {                                                                                          \
            diagnostics.first_move_makes_cut.update(TYPE == NodeType::Cut);                        \
            scratchpad_.first_move_score = SCORE;                                                  \
        }                                                                                          \
        if (scratchpad_.processing_tt_move)                                                        \
        {                                                                                          \
            diagnostics.tt_move_makes_cut.update(TYPE == NodeType::Cut);                           \
            scratchpad_.tt_move_score = SCORE;                                                     \
            scratchpad_.processing_tt_move = false;                                                \
        }                                                                                          \
        if (scratchpad_.processing_killer_move)                                                    \
        {                                                                                          \
            diagnostics.killer_move_makes_cut.update(TYPE == NodeType::Cut);                       \
            scratchpad_.killer_move_score = SCORE;                                                 \
            scratchpad_.processing_killer_move = false;                                            \
        }                                                                                          \
    } while (false)

#endif

}  // namespace rock::internal
