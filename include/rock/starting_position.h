#pragma once
#include "parse.h"

namespace rock
{

ROCK_API inline constexpr auto starting_board = parse_literal_board(" wwwwww "
                                                                    "b      b"
                                                                    "b      b"
                                                                    "b      b"
                                                                    "b      b"
                                                                    "b      b"
                                                                    "b      b"
                                                                    " wwwwww ");

ROCK_API inline constexpr auto starting_position = Position{starting_board, Player::White};

}  // namespace rock
