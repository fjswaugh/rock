#pragma once

#include "common.h"
#include <cassert>
#include <optional>
#include <vector>

namespace rock
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// Player
////////////////////////////////////////////////////////////////////////////////////////////////////

enum struct ROCK_API Player : bool
{
    White,
    Black,
};

ROCK_API constexpr auto opponent_of(Player p) -> Player
{
    return Player{!static_cast<bool>(p)};
}

ROCK_API constexpr auto operator!(Player p) -> Player
{
    return opponent_of(p);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Board
////////////////////////////////////////////////////////////////////////////////////////////////////

struct ROCK_API BitBoard;

struct ROCK_API BoardCoordinates
{
    using underlying_type = u8;

    constexpr BoardCoordinates() = default;

    template <typename T>
    explicit constexpr BoardCoordinates(T data) : data_{static_cast<underlying_type>(data)}
    {
#ifndef NDEBUG
        if (data_ >= underlying_type{64})
            throw "Out of bounds in BoardCoordinates";
#endif
    }

    template <typename T>
    constexpr BoardCoordinates(T x, T y) : BoardCoordinates{static_cast<underlying_type>(y * 8 + x)}
    {}

    constexpr auto data() const -> underlying_type { return data_; }
    explicit constexpr operator underlying_type() const { return data_; }

    constexpr auto x() const -> int { return static_cast<int>(data_) % 8; }
    constexpr auto y() const -> int { return static_cast<int>(data_) / 8; }

    constexpr auto bit_board() const -> BitBoard;

    friend constexpr auto operator<(BoardCoordinates p1, BoardCoordinates p2) -> bool
    {
        return p1.data_ < p2.data_;
    }
    friend constexpr auto operator<=(BoardCoordinates p1, BoardCoordinates p2) -> bool
    {
        return p1.data_ <= p2.data_;
    }
    friend constexpr auto operator==(BoardCoordinates p1, BoardCoordinates p2) -> bool
    {
        return p1.data_ == p2.data_;
    }
    friend constexpr auto operator!=(BoardCoordinates p1, BoardCoordinates p2) -> bool
    {
        return p1.data_ != p2.data_;
    }

private:
    underlying_type data_{};
};

struct ROCK_API BitBoard
{
    constexpr BitBoard() = default;
    constexpr BitBoard(u64 data) : data{data} {}
    constexpr operator u64() const { return data; }
    constexpr operator u64&() { return data; }

    constexpr auto at(BoardCoordinates c) const -> bool { return data & c.bit_board(); }

    constexpr auto set_bit(BoardCoordinates c) -> void { data |= c.bit_board(); }
    constexpr auto flip_bit(BoardCoordinates c) -> void { data ^= c.bit_board(); }
    constexpr auto clear_bit(BoardCoordinates c) -> void { data &= ~c.bit_board(); }

    auto extract_one() -> BitBoard;
    auto count() const -> std::size_t;
    auto coordinates() const -> BoardCoordinates;

    u64 data{};
};

constexpr auto BoardCoordinates::bit_board() const -> BitBoard
{
    return BitBoard{u64{1} << u64{data_}};
}

struct ROCK_API Board
{
    constexpr Board() = default;
    constexpr Board(BitBoard white, BitBoard black) : boards_{white, black} {}

    constexpr auto pieces_for(Player p) -> BitBoard& { return boards_[static_cast<bool>(p)]; }
    constexpr auto operator[](Player p) -> BitBoard& { return boards_[static_cast<bool>(p)]; }

    constexpr auto pieces_for(Player p) const -> BitBoard { return boards_[static_cast<bool>(p)]; }
    constexpr auto operator[](Player p) const -> BitBoard { return boards_[static_cast<bool>(p)]; }

private:
    BitBoard boards_[2];
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Position
////////////////////////////////////////////////////////////////////////////////////////////////////

struct ROCK_API Position
{
    constexpr Position() = default;
    constexpr Position(Board const& board, Player player_to_move)
        : board_{board}, player_to_move_{player_to_move}
    {}

    constexpr auto friends() const -> BitBoard { return board_[player_to_move_]; }
    constexpr auto enemies() const -> BitBoard { return board_[!player_to_move_]; }

    constexpr auto board() const -> Board const& { return board_; }
    constexpr auto player_to_move() const -> Player { return player_to_move_; }

    constexpr auto set_board(Board const& b) -> void { board_ = b; }
    constexpr auto set_player_to_move(Player p) -> void { player_to_move_ = p; }

private:
    Board board_{};
    Player player_to_move_{};
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Move
////////////////////////////////////////////////////////////////////////////////////////////////////

struct ROCK_API Move
{
    BoardCoordinates from;
    BoardCoordinates to;

    constexpr auto friend operator==(Move m1, Move m2) -> bool
    {
        return m1.from == m2.from && m1.to == m2.to;
    }

    constexpr auto friend operator<(Move m1, Move m2) -> bool
    {
        return (m1.from == m2.from) ? (m1.to < m2.to) : (m1.from < m2.from);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// GameOutcome
////////////////////////////////////////////////////////////////////////////////////////////////////

enum struct ROCK_API GameOutcome
{
    Ongoing = 0,
    WhiteWins,
    BlackWins,
    Draw,
};

using ScoreType = std::int64_t;

struct ROCK_API PositionAnalysis
{
    std::optional<Move> best_move;
    std::vector<Move> principal_variation;
    ScoreType score;
};

}  // namespace rock
