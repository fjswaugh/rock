#pragma once

#include "common.h"
#include <cassert>

namespace rock
{

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
    return Player{!static_cast<bool>(p)};
}

enum struct GameOutcome
{
    Ongoing = 0,
    WhiteWins,
    BlackWins,
    Draw,
};

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
    explicit constexpr operator u8() const { return data_; };

    constexpr auto x() const -> int { return data_ % 8; }
    constexpr auto y() const -> int { return data_ / 8; }

    constexpr auto board() const -> BitBoard;

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

    constexpr auto at(BoardPosition pos) const -> bool { return data & pos.board(); }

    constexpr auto set_bit(BoardPosition pos) -> void { data |= pos.board(); }
    constexpr auto flip_bit(BoardPosition pos) -> void { data ^= pos.board(); }
    constexpr auto clear_bit(BoardPosition pos) -> void { data &= ~pos.board(); }

    u64 data{};
};

constexpr auto BoardPosition::board() const -> BitBoard
{
    return BitBoard{u64{1} << u64{data_}};
}

struct Board
{
    constexpr auto pieces_for(Player p) -> BitBoard& { return boards_[static_cast<bool>(p)]; }
    constexpr auto operator[](Player p) -> BitBoard& { return boards_[static_cast<bool>(p)]; }

    constexpr auto pieces_for(Player p) const -> BitBoard { return boards_[static_cast<bool>(p)]; }
    constexpr auto operator[](Player p) const -> BitBoard { return boards_[static_cast<bool>(p)]; }

private:
    BitBoard boards_[2];
};

struct Move
{
    BoardPosition from;
    BoardPosition to;

    constexpr auto friend operator==(Move m1, Move m2) -> bool
    {
        return m1.from == m2.from && m1.to == m2.to;
    }
};

struct MoveList
{
    constexpr void push_back(Move move)
    {
        assert(size_ != 12 * 8);
        moves_[size_++] = move;
    }
    constexpr auto begin() const { return &moves_[0]; }
    constexpr auto end() const { return this->begin() + size_; }
    constexpr auto size() const { return size_; }

    constexpr Move& operator[](std::size_t i) { return moves_[i]; }
    constexpr Move operator[](std::size_t i) const { return moves_[i]; }

private:
    constexpr static auto max_size = 12 * 8;

    Move moves_[max_size];
    std::size_t size_{};
};

constexpr auto apply_move(Move const m, Board b, Player const player) -> Board
{
    auto const from = m.from.board();
    auto const to = m.to.board();

    b[player] ^= (from | to);
    b[!player] &= ~to;

    return b;
}

}  // namespace rock
