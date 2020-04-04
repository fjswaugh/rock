#include "parse.h"

namespace rock
{

constexpr auto starting_board = parse_literal_board(" wwwwww "
                                                    "b      b"
                                                    "b      b"
                                                    "b      b"
                                                    "b      b"
                                                    "b      b"
                                                    "b      b"
                                                    " wwwwww ");

constexpr auto starting_position = Position{starting_board, Player::White};

}  // namespace rock
