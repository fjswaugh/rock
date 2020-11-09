#include <fmt/format.h>
#include <rock/algorithms.h>
#include <rock/fen.h>
#include <rock/format.h>
#include <iostream>
#include <string_view>

using namespace fmt::literals;

auto main(int argc, char** argv) -> int
{
    if (argc != 2)
    {
        std::cerr << fmt::format("Bad number of arguments ({} instead of 2)\n", argc);
        return 1;
    }

    auto fen_str = std::string_view{argv[1]};
    std::optional position = rock::parse_fen_to_position(fen_str);

    if (!position)
    {
        std::cerr << "Fen parse error\n";
        return 2;
    }

    if (auto const outcome = rock::get_game_outcome(*position);
        outcome != rock::GameOutcome::Ongoing)
    {
        std::cout << fmt::format("Game over: {}\n", outcome);
        return 0;
    }

    auto const analysis = rock::analyze_position(*position, 8);

    if (!analysis.best_move)
    {
        std::cerr << "Cannot find best move, even though game is not over...\n";
        return 3;
    }

    auto const new_position = rock::apply_move(*analysis.best_move, *position);

    std::cout << fmt::format(
        "{}\n{}\n[{}]\n{}\n--------\n",
        *analysis.best_move,
        analysis.score,
        fmt::join(analysis.principal_variation.begin(), analysis.principal_variation.end(), ", "),
        rock::format_as_fen(new_position));

    return 0;
}
