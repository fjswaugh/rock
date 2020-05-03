#pragma once

#include "bit_operations.h"
#include "rock/algorithms.h"
#include "rock/types.h"
#include <optional>

namespace rock::internal
{

enum struct NodeType
{
    All,
    Pv,
    Cut,
};

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

}  // namespace rock::internal
