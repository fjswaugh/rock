#include <fmt/format.h>
#include <rock/algorithms.h>
#include <rock/common.h>
#include <rock/format.h>
#include <rock/parse.h>
#include <rock/starting_board.h>
#include <rock/types.h>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <random>
#include <regex>
#include <string_view>
#include <vector>

auto print_random_game(std::mt19937& rng) -> void
{
    auto line = std::string{};
    auto b = rock::starting_board;
    auto i = int{};
    while (std::getline(std::cin, line))
    {
        {
            auto const move = pick_random_move(b, rock::Color::White, rng);
            b = apply_move(move, b, rock::Color::White);
            std::cout << fmt::format("White: {}\n", move);
        }

        {
            auto const move = pick_random_move(b, rock::Color::Black, rng);
            b = apply_move(move, b, rock::Color::Black);
            std::cout << fmt::format("Black: {}\n", move);
        }

        std::cout << fmt::format("Turn {}:\n{}\n", ++i, b);
        if (rock::are_pieces_all_together(b.pieces[0]) ||
            rock::are_pieces_all_together(b.pieces[1]))
        {
            std::cout << "End.\n";
            break;
        }
    }
}

struct GameInfo
{
    rock::Color winner{};
    int num_turns{};
};

auto play_random_game(std::mt19937& rng) -> GameInfo
{
    auto const test_winner = [&](rock::Board const& b) -> std::optional<rock::Color> {
        if (rock::are_pieces_all_together(b.pieces[0]))
            return rock::Color(0);
        else if (rock::are_pieces_all_together(b.pieces[1]))
            return rock::Color(1);
        return std::nullopt;
    };

    auto c = rock::Color::White;
    auto b = rock::starting_board;
    auto i = int{};

    while (true)
    {
        auto move = rock::pick_random_move(b, c, rng);

        b = apply_move(move, b, c);
        ++i;
        c = rock::Color(!bool(c));

        if (auto winner = test_winner(b))
            return {*winner, i};
    }
}

auto main(int argc, char** argv) -> int
{
    if (argc == 2)
    {
        auto const n = std::stoi(argv[1]);
        constexpr auto const player_to_move = rock::Color::White;
        std::cout << fmt::format(
            "Number of moves: {}\n", count_moves(rock::starting_board, player_to_move, n));
    }

    auto rng = std::mt19937{std::random_device{}()};

    if (false)
    {
        print_random_game(rng);
    }

    if (false)
    {
        for (auto i = 0; i < 1000; ++i)
        {
            auto const res = play_random_game(rng);
            std::cout << fmt::format(
                "Winner: '{}', Num turns: {}\n",
                res.winner == rock::Color::White ? "w" : "b",
                res.num_turns);
        }
    }
}
