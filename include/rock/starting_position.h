#pragma once
#include "parse.h"

namespace rock
{

inline constexpr auto starting_board = parse_literal_board(" wwwwww "
                                                           "b      b"
                                                           "b      b"
                                                           "b      b"
                                                           "b      b"
                                                           "b      b"
                                                           "b      b"
                                                           " wwwwww ");

inline constexpr auto starting_position = Position{starting_board, Player::White};

}  // namespace rock
