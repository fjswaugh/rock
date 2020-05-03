#pragma once

#include "diagnostics.h"
#include "internal_types.h"
#include "rock/types.h"
#include <absl/hash/hash.h>

#ifdef NO_USE_CUSTOM_TT
#include <absl/container/flat_hash_map.h>
#endif

namespace rock::internal
{

#ifndef NO_USE_CUSTOM_TT
inline auto compute_hash(u64 friends, u64 enemies) -> std::size_t
{
    auto const key = std::tuple{friends, enemies};
    return absl::Hash<decltype(key)>{}(key);
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

    static constexpr auto size = std::size_t{20};

    TranspositionTable() : data_(std::size_t{2} << size, Value{}) {}

    auto reset() -> void { std::fill(data_.begin(), data_.end(), Value{}); }

    struct LookupResult
    {
        Value* value;
        bool was_found;
    };

    auto lookup(u64 friends, u64 enemies) -> LookupResult
    {
        auto const index = compute_hash(friends, enemies) % (std::size_t{2} << size);
        auto* value = &data_[index];
#ifdef DIAGNOSTICS
        bool const is_collision = (value->friends_ != 0 || value->enemies_ != 0) &&
            !value->matches(friends, enemies);
        diagnostics.tt_hash_collisions.update(is_collision);
#endif
        return {value, value->matches(friends, enemies)};
    }

private:
    std::vector<Value> data_{};
};
#else
struct TranspositionTable
{
public:
    using Key = std::pair<u64, u64>;

    struct Value
    {
        constexpr auto set_key(u64, u64) -> void {}

        InternalMoveRecommendation recommendation{};
        int depth{};
        NodeType type{};
    };

    TranspositionTable() = default;

    auto reset() -> void { data_ = {}; }

    struct LookupResult
    {
        Value* value;
        bool was_found;
    };

    auto lookup(u64 friends, u64 enemies) -> LookupResult
    {
        auto const [it, did_insert] = data_.try_emplace(std::pair{friends, enemies});
        return {&it->second, !did_insert};
    }

private:
    absl::flat_hash_map<Key, Value> data_{};
};
#endif

}  // namespace rock::internal
