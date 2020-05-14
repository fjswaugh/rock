#pragma once

#include "types.h"
#include <optional>
#include <string>
#include <string_view>

namespace rock
{

ROCK_API auto format_as_fen(rock::Board const&) -> std::string;
ROCK_API auto parse_fen_to_board(std::string_view fen_string) -> std::optional<rock::Board>;

}  // namespace rock
