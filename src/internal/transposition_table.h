#pragma once

#include "bit_operations.h"
#include "diagnostics.h"
#include "internal_types.h"
#include "rock/types.h"

namespace rock::internal
{

// Taken from:
// https://stackoverflow.com/questions/664014/what-integer-hash-function-are-good-that-accepts-an-integer-hash-key
// 
// Which was in turn taken from split mix 64:
// http://xorshift.di.unimi.it/splitmix64.c
//
// This article may be relevant:
// https://zimbry.blogspot.com/2011/09/better-bit-mixing-improving-on.html
inline auto compute_hash(u64 friends, u64 enemies) -> u64
{
    auto x = friends;
    x = (x ^ (x >> 30)) * u64{0xbf58476d1ce4e5b9};
    x = (x ^ (x >> 27)) * u64{0x94d049bb133111eb};
    x = x ^ (x >> 31);

    auto y = enemies;
    y = (y ^ (y >> 30)) * u64{0xbf58476d1ce4e5b9};
    y = (y ^ (y >> 27)) * u64{0x94d049bb133111eb};
    y = y ^ (y >> 31);

    return x ^ y;
}

struct TranspositionTable
{
public:
    struct Value
    {
    private:
        friend struct TranspositionTable;

        u64 friends_{};
        u64 enemies_{};

        constexpr auto matches(u64 f, u64 e) const -> bool
        {
            return f == friends_ && e == enemies_;
        }

    public:
        constexpr auto set_key(u64 friends, u64 enemies) -> void
        {
            friends_ = friends;
            enemies_ = enemies;
        }

        InternalMoveRecommendation recommendation{};
        int depth{};
        NodeType type{};
    };

    static constexpr auto default_size = std::size_t{16};

    explicit TranspositionTable(std::size_t size = default_size)
        : size_{size}, data_(std::size_t{2} << size_, Value{})
    {}

    auto reset() -> void { std::fill(data_.begin(), data_.end(), Value{}); }

    struct LookupResult
    {
        Value* value;
        bool was_found;
    };

    struct ConstLookupResult
    {
        Value const* value;
        bool was_found;
    };

    auto lookup(u64 friends, u64 enemies) const -> ConstLookupResult
    {
        auto const index = compute_hash(friends, enemies) % (std::size_t{2} << size_);
        auto const* value = &data_[index];
        return {value, value->matches(friends, enemies)};
    }

    auto lookup(u64 friends, u64 enemies) -> LookupResult
    {
        auto const index = compute_hash(friends, enemies) % (std::size_t{2} << size_);
        auto* value = &data_[index];
#ifdef DIAGNOSTICS
        bool const is_collision = (value->friends_ != 0 || value->enemies_ != 0) &&
            !value->matches(friends, enemies);
        diagnostics.tt_hash_collisions.update(is_collision);
#endif
        return {value, value->matches(friends, enemies)};
    }

private:
    std::size_t size_{};
    std::vector<Value> data_{};
};

}  // namespace rock::internal
