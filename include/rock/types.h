#pragma once

#include "common.h"
#include <cassert>

namespace rock
{

////////////////////////////////////////////////////////////////////////////////////////////////////
// Player
////////////////////////////////////////////////////////////////////////////////////////////////////

enum struct Player : bool
{
    White,
    Black,
};

constexpr auto opponent_of(Player p) -> Player
{
    return Player{!static_cast<bool>(p)};
}

constexpr auto operator!(Player p) -> Player
{
    return opponent_of(p);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Board
////////////////////////////////////////////////////////////////////////////////////////////////////

struct BitBoard;

struct BoardPosition
{
    constexpr BoardPosition() = default;
    template <typename T>
    explicit constexpr BoardPosition(T data) : data_{static_cast<u8>(data)}
    {
#ifndef NDEBUG
        if (data >= static_cast<T>(64))
            throw "Out of bounds in BoardPosition";
#endif
    }
    template <typename T>
    constexpr BoardPosition(T x, T y) : BoardPosition{static_cast<u8>(y * 8 + x)}
    {}

    constexpr auto data() const -> u8 { return data_; }
    explicit constexpr operator u8() const { return data_; }

    constexpr auto x() const -> int { return data_ % 8; }
    constexpr auto y() const -> int { return data_ / 8; }

    constexpr auto bit_board() const -> BitBoard;

    friend constexpr auto operator<(BoardPosition p1, BoardPosition p2) -> bool
    {
        return p1.data_ < p2.data_;
    }
    friend constexpr auto operator<=(BoardPosition p1, BoardPosition p2) -> bool
    {
        return p1.data_ <= p2.data_;
    }
    friend constexpr auto operator==(BoardPosition p1, BoardPosition p2) -> bool
    {
        return p1.data_ == p2.data_;
    }
    friend constexpr auto operator!=(BoardPosition p1, BoardPosition p2) -> bool
    {
        return p1.data_ != p2.data_;
    }

private:
    u8 data_{};
};

struct BitBoard
{
    constexpr BitBoard() = default;
    constexpr BitBoard(u64 data) : data{data} {}
    constexpr operator u64() const { return data; }
    constexpr operator u64&() { return data; }

    constexpr auto at(BoardPosition pos) const -> bool { return data & pos.bit_board(); }

    constexpr auto set_bit(BoardPosition pos) -> void { data |= pos.bit_board(); }
    constexpr auto flip_bit(BoardPosition pos) -> void { data ^= pos.bit_board(); }
    constexpr auto clear_bit(BoardPosition pos) -> void { data &= ~pos.bit_board(); }

    u64 data{};
};

constexpr auto BoardPosition::bit_board() const -> BitBoard
{
    return BitBoard{u64{1} << u64{data_}};
}

struct Board
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

struct Position
{
    constexpr Position(Board const& board, Player player_to_move)
        : board_{board}, player_to_move_{player_to_move}
    {}

    constexpr auto board() const -> Board const& { return board_; }
    constexpr auto player_to_move() const -> Player { return player_to_move_; }

    constexpr auto set_board(Board const& b) -> void { board_ = b; }
    constexpr auto set_player_to_move(Player p) -> void { player_to_move_ = p; }

private:
    Board board_;
    Player player_to_move_;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// Move
////////////////////////////////////////////////////////////////////////////////////////////////////

struct Move
{
    BoardPosition from;
    BoardPosition to;

    constexpr auto friend operator==(Move m1, Move m2) -> bool
    {
        return m1.from == m2.from && m1.to == m2.to;
    }
};

constexpr auto apply_move(Move const m, Board b, Player const player) -> Board
{
    auto const from = m.from.bit_board();
    auto const to = m.to.bit_board();

    b[player] ^= (from | to);
    b[!player] &= ~to;

    return b;
}

constexpr auto apply_move(Move const m, Position p) -> Position
{
    p.set_board(apply_move(m, p.board(), p.player_to_move()));
    p.set_player_to_move(!p.player_to_move());
    return p;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// GameOutcome
////////////////////////////////////////////////////////////////////////////////////////////////////

enum struct GameOutcome
{
    Ongoing = 0,
    WhiteWins,
    BlackWins,
    Draw,
};

}  // namespace rock
