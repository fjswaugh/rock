#pragma once

#include "rock/format.h"
#include <doctest/doctest.h>
#include <string>
#include <vector>
#include <type_traits>

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
        auto str = fmt::format("[{}]", fmt::join(v.begin(), v.end(), ", "));
        return String(str.c_str());
    }
};

}  // namespace doctest
