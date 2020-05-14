#pragma once

#include "types.h"
#include <optional>
#include <vector>

namespace rock
{

struct ROCK_API Game
{
    Game();

    static auto from_position(Position const&) -> Game;
    static auto standard_new_game() -> Game;

    auto num_moves_played() const -> std::size_t;
    auto size() const -> std::size_t;
    auto is_empty() const -> bool;

    auto current_position() const -> Position const&;
    auto current_status() const -> GameOutcome;
    auto most_recent_move() const -> Move;

    /**
     * Returns true if move is legal
     */
    auto make_move(Move) -> bool;

    auto undo_move() -> std::optional<Move>;
    auto redo_move() -> std::optional<Move>;
    auto reset_to_start() -> void;

private:
    Game(std::vector<Position>, std::vector<Move>);

    std::vector<Position> history_{};
    std::vector<Move> moves_{};
    std::size_t i_{};
};

}  // namespace rock
