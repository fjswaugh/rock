#pragma once

#include "rock/types.h"
#include <limits>

namespace rock::internal
{

[[maybe_unused]] inline auto pop_count_manual(u64 x) -> u64
{
    static constexpr int const s[] = {1, 2, 4, 8, 16, 32};
    static constexpr u64 const b[] = {
        0x5555555555555555,
        0x3333333333333333,
        0x0F0F0F0F0F0F0F0F,
        0x00FF00FF00FF00FF,
        0x0000FFFF0000FFFF,
        0x00000000FFFFFFFF,
    };

    // clang-format off
    u64 c;
    c = x - ((x >> u64{1}) & b[0]);
    c = ((c >> s[1]) & b[1]) + (c & b[1]);
    c = ((c >> s[2]) + c) & b[2];
    c = ((c >> s[3]) + c) & b[3];
    c = ((c >> s[4]) + c) & b[4];
    c = ((c >> s[5]) + c) & b[5];
    return c;
    // clang-format on
}

[[maybe_unused]] inline auto count_trailing_zeros_manual(u64 x) -> u64
{
    // clang-format off
    auto c = u64{64};
    x &= static_cast<u64>(-static_cast<std::int64_t>(x));
    if (x) c--;
    if (x & u64{0x00000000FFFFFFFF}) c -= 32;
    if (x & u64{0x0000FFFF0000FFFF}) c -= 16;
    if (x & u64{0x00FF00FF00FF00FF}) c -=  8;
    if (x & u64{0x0F0F0F0F0F0F0F0F}) c -=  4;
    if (x & u64{0x3333333333333333}) c -=  2;
    if (x & u64{0x5555555555555555}) c -=  1;
    return c;
    // clang-format on
}

inline auto pop_count(u64 x) -> u64
{
#if defined(__GNUC__)
    if constexpr (std::numeric_limits<unsigned long>::digits == 64)
        return static_cast<u64>(__builtin_popcountl(x));
    else if constexpr (std::numeric_limits<unsigned long long>::digits == 64)
        return static_cast<u64>(__builtin_popcountll(x));
    else
#endif
        return pop_count_manual(x);
}

inline auto coordinates_from_bit_board(u64 b) -> u64
{
#if defined(__GNUC__)
    if constexpr (std::numeric_limits<unsigned long>::digits == 64)
        return static_cast<u64>(__builtin_ctzl(b));
    else if constexpr (std::numeric_limits<unsigned long long>::digits == 64)
        return static_cast<u64>(__builtin_ctzll(b));
    else
#endif
        return count_trailing_zeros_manual(b);
}

inline auto bit_board_from_coordinates(u64 c) -> u64
{
    return u64{1} << c;
}

inline auto extract_one_bit(u64& x) -> u64
{
    auto const c = coordinates_from_bit_board(x);
    auto const board = bit_board_from_coordinates(c);
    x ^= board;
    return board;
}

}  // namespace rock
