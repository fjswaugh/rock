#pragma once

#include "common.h"
#include <cassert>

namespace rock
{

enum struct Color : bool
{
    White,
    Black,
};

struct BitBoard;

struct BoardPosition
{
    constexpr BoardPosition() = default;
    template <typename T>
    constexpr BoardPosition(T data) : data_{static_cast<u8>(data)}
    {
#ifndef NDEBUG
        if (data >= static_cast<T>(64))
            throw "Out of bounds in BoardPosition";
#endif
    }
    template <typename T>
    constexpr BoardPosition(T x, T y) : BoardPosition{static_cast<u8>(y * 8 + x)}
    {}

    static constexpr auto min() -> BoardPosition { return BoardPosition{}; }
    static constexpr auto max() -> BoardPosition { return BoardPosition{63}; }

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
    friend constexpr auto operator++(BoardPosition& p) -> BoardPosition&
    {
        ++p.data_;
        return p;
    }
    friend constexpr auto operator++(BoardPosition& p, int) -> BoardPosition
    {
        auto const res = p;
        ++p.data_;
        return res;
    }

private:
    u8 data_{};
};

struct BitBoard
{
    constexpr BitBoard() = default;
    constexpr BitBoard(u64 data) : data_{data} {}

    constexpr auto data() const -> u64 { return data_; }
    constexpr operator u64() const { return data_; }

    constexpr auto at(BoardPosition pos) const -> bool { return data_ & pos.board().data(); }

    constexpr auto set_bit(BoardPosition pos) -> void { data_ |= pos.board().data(); }
    constexpr auto flip_bit(BoardPosition pos) -> void { data_ ^= pos.board().data(); }
    constexpr auto clear_bit(BoardPosition pos) -> void { data_ &= (~pos.board().data()); }

private:
    u64 data_{};
};

constexpr auto BoardPosition::board() const -> BitBoard
{
    return BitBoard{u64{1} << u64{data_}};
}

struct Board
{
    u64 pieces[2];
};

struct Move
{
    u8 from;
    u8 to;

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

constexpr auto apply_move(Move const m, Board b, Color const player) -> Board
{
    // TODO: tidy up
    auto const from = BoardPosition{m.from}.board().data();
    auto const to = BoardPosition{m.to}.board().data();

    b.pieces[bool(player)] ^= (from | to);
    b.pieces[!bool(player)] &= ~to;

    return b;
}

}  // namespace rock
