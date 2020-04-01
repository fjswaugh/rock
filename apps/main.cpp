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

auto pick_random_move(rock::Board const& b, rock::Player p, std::mt19937& rng)
{
    auto const all_moves = rock::generate_moves(b, p);
    auto dist = std::uniform_int_distribution<std::size_t>{0, all_moves.size() - 1};
    return all_moves[dist(rng)];
}

auto print_random_game(std::mt19937& rng) -> void
{
    auto line = std::string{};
    auto b = rock::starting_board;
    auto i = int{};
    while (std::getline(std::cin, line))
    {
        {
            auto const move = pick_random_move(b, rock::Player::White, rng);
            b = apply_move(move, b, rock::Player::White);
            std::cout << fmt::format("White: {}\n", move);
        }

        {
            auto const move = pick_random_move(b, rock::Player::Black, rng);
            b = apply_move(move, b, rock::Player::Black);
            std::cout << fmt::format("Black: {}\n", move);
        }

        std::cout << fmt::format("Turn {}:\n{}\n", ++i, b);
        if (rock::are_pieces_all_together(b[rock::Player::White]) ||
            rock::are_pieces_all_together(b[rock::Player::Black]))
        {
            std::cout << "End.\n";
            break;
        }
    }
}

struct GameInfo
{
    rock::GameOutcome outcome{};
    int num_turns{};
};

auto play_random_game(std::mt19937& rng) -> GameInfo
{
    auto p = rock::Player::White;
    auto b = rock::starting_board;
    auto i = int{};

    while (true)
    {
        auto move = pick_random_move(b, p, rng);

        b = apply_move(move, b, p);
        ++i;
        p = !p;

        if (auto outcome = rock::get_game_outcome(b, p); outcome != rock::GameOutcome::Ongoing)
            return {outcome, i};
    }
}

auto main(int argc, char** argv) -> int
{
    if (argc == 2)
    {
        auto const n = std::stoi(argv[1]);
        constexpr auto const player_to_move = rock::Player::White;
        std::cout << fmt::format(
            "Number of moves: {}\n", count_moves(rock::starting_board, player_to_move, n));
    }

    std::cout << fmt::format("Number of moves: {}\n", count_moves(rock::starting_board, rock::Player::White, 5));

    auto rng = std::mt19937{std::random_device{}()};

    //print_random_game(rng);

    //for (auto i = 0; i < 1000; ++i)
    //{
    //    auto const res = play_random_game(rng);
    //    std::cout << fmt::format("Outcome: '{}', Num turns: {}\n", res.outcome, res.num_turns);
    //}
}
