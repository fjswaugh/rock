#include <string>
#include <cxxopts.hpp>
#include <iostream>

auto main(int argc, char** argv) -> int
try
{
    auto options = cxxopts::Options(
        "rock_play_against_each_other",
        "Play multiple versions of Romanian Checkers AIs against each other");

    // clang-format off
    options.add_options()
        ("h,help", "Print usage")
        ("x,player1", "Program path for player 1", cxxopts::value<std::string>())
        ("y,player2", "Program path for player 2", cxxopts::value<std::string>())
    ;
    // clang-format on

    auto result = options.parse(argc, argv);

    if (result.count("help"))
    {
        std::cout << options.help() << '\n';
        return 0;
    }

    auto x = result["x"].as<std::string>();
    auto y = result["y"].as<std::string>();


}
catch (cxxopts::OptionException const& e)
{
    std::cerr << "Error: " << e.what() << '\n';
    return 1;
}
