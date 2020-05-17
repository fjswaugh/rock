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

template <>
struct StringMaker<rock::Position>
{
    inline static String convert(rock::Position const& p)
    {
        auto str = fmt::format("{:e+}", p);
        return String(str.c_str());
    }
};

template <>
struct StringMaker<rock::Board>
{
    inline static String convert(rock::Board const& b)
    {
        auto str = fmt::format("{:e+}", b);
        return String(str.c_str());
    }
};

template <>
struct StringMaker<rock::BitBoard>
{
    inline static String convert(rock::BitBoard const& b)
    {
        auto str = fmt::format("{}", b);
        return String(str.c_str());
    }
};

template <typename T>
struct StringMaker<std::optional<T>>
{
    inline static String convert(std::optional<T> const& o)
    {
        if (!o)
            return "nullopt";
        else
            return StringMaker<T>::convert(*o);
    }
};

}  // namespace doctest
