#pragma once

#include "rock/format.h"
#include <doctest/doctest.h>
#include <string>
#include <vector>
#include <type_traits>

namespace rock::detail
{

template <typename T>
struct is_iterable : std::false_type
{};

template <typename T>
constexpr bool is_iterable_v = is_iterable<T>::value;

}  // namespace rock::detail

namespace doctest
{

template <>
struct StringMaker<rock::BoardCoordinates>
{
    inline static String convert(rock::BoardCoordinates c) { return rock::to_string(c).c_str(); }
};

template <typename T>
struct StringMaker<std::vector<T>>
{
    inline static String convert(std::vector<T> const& v)
    {
        auto str = String{"["};
        for (auto i = std::size_t{}; i != v.size(); ++i)
            str += StringMaker<T>::convert(v[i]) + (i == v.size() - 1 ? "" : ", ");
        str += "]";
        return str;
    }
};

}  // namespace doctest
