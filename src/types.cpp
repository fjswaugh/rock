#include "rock/types.h"
#include "bit_operations.h"

namespace rock
{

auto BitBoard::extract_one() -> BitBoard
{
    return extract_one_bit(data);
}

auto BitBoard::count() const -> std::size_t
{
    return pop_count(data);
}

auto BitBoard::coordinates() const -> BoardCoordinates
{
    return BoardCoordinates{coordinates_from_bit_board(data)};
}

}  // namespace rock
