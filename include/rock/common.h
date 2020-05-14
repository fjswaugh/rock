#pragma once

#include <cstdint>
#include <cstddef>

namespace rock
{

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

}  // namespace rock

#if defined(_WIN32)
#  ifdef ROCK_EXPORT
#    define ROCK_API __declspec(dllexport)
#  elif defined(ROCK_SHARED)
#    define ROCK_API __declspec(dllimport)
#  endif
#endif
#ifndef ROCK_API
#  if defined(__GNUC__)
#    define ROCK_API __attribute__((visibility("default")))
#  else
#    define ROCK_API
#  endif
#endif
