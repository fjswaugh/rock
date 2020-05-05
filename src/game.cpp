#include "rock/game.h"
#include "rock/algorithms.h"
#include "rock/starting_position.h"
#include <cassert>

namespace rock
{

Game::Game() = default;

Game::Game(std::vector<Position> history, std::vector<Move> moves)
    : history_{std::move(history)}, moves_{std::move(moves)}
{}

auto Game::from_position(Position const& position) -> Game
{
    return Game{{position}, {}};
}

auto Game::standard_new_game() -> Game
{
    return Game{{rock::starting_position}, {}};
}

auto Game::num_moves_played() const -> std::size_t
{
    return moves_.size();
}

auto Game::size() const -> std::size_t
{
    return history_.size();
}

auto Game::is_empty() const -> bool
{
    return history_.empty();
}

auto Game::current_position() const -> Position const&
{
    return history_[i_];
}

auto Game::current_status() const -> GameOutcome
{
    return rock::get_game_outcome(current_position());
}

auto Game::most_recent_move() const -> Move
{
    assert(i_ > 0);
    return moves_[i_ - 1];
}

auto Game::make_move(Move move) -> bool
{
    assert(!history_.empty());

    if (!is_move_legal(move, current_position()))
        return false;

    history_.erase(history_.begin() + static_cast<std::ptrdiff_t>(i_) + 1, history_.end());
    moves_.erase(moves_.begin() + static_cast<std::ptrdiff_t>(i_), moves_.end());

    moves_.push_back(move);
    history_.push_back(apply_move(move, current_position()));
    ++i_;

    return true;
}

auto Game::undo_move() -> std::optional<Move>
{
    if (i_ == 0)
        return std::nullopt;

    auto const move = most_recent_move();
    --i_;
    return move;
}

auto Game::redo_move() -> std::optional<Move>
{
    if (i_ + 1 >= size())
        return std::nullopt;

    ++i_;
    return most_recent_move();
}

auto Game::reset_to_start() -> void
{
    i_ = 0;
}

}  // namespace rock
