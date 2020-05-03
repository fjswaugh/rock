#include "rock/types.h"
#include "internal/bit_operations.h"

namespace rock
{

auto BitBoard::extract_one() -> BitBoard
{
    return internal::extract_one_bit(data);
}

auto BitBoard::count() const -> std::size_t
{
    return internal::pop_count(data);
}

auto BitBoard::coordinates() const -> BoardCoordinates
{
    return BoardCoordinates{internal::coordinates_from_bit_board(data)};
}

}  // namespace rock
