#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <rock/algorithms.h>
#include <rock/format.h>
#include <rock/starting_board.h>
#include <algorithm>

using namespace pybind11::literals;

namespace
{

auto py_apply_move(rock::Board& x, rock::Move m) -> void
{
    auto const player = rock::BitBoard{x[rock::Player::White]}.at(m.from)
        ? rock::Player::White
        : rock::Player::Black;

    x = rock::apply_move(m, x, player);
}

auto py_format_board(rock::Board const& x, std::string const& format_string) -> std::string
{
    auto const full_format_string = format_string.empty() ? "{}" : "{:" + format_string + "}";
    return fmt::format(full_format_string, x);
}

}  // namespace

PYBIND11_MODULE(rock, m)
{
    m.doc() = "Rock library for Romanian Checkers";

    // Types

    pybind11::enum_<rock::Player>(m, "Player")
        .value("White", rock::Player::White)
        .value("Black", rock::Player::Black)
        .export_values();

    pybind11::enum_<rock::GameOutcome>(m, "GameOutcome")
        .value("Ongoing", rock::GameOutcome::Ongoing)
        .value("WhiteWins", rock::GameOutcome::WhiteWins)
        .value("BlackWins", rock::GameOutcome::BlackWins)
        .value("Draw", rock::GameOutcome::Draw)
        .export_values();

    pybind11::class_<rock::BoardPosition>(m, "BoardPosition")
        .def(pybind11::init<>())
        .def(pybind11::init(
            [](std::string const& str) { return rock::parse_board_position(str).value(); }))
        .def(pybind11::init([](int x, int y) {
            return rock::BoardPosition{x, y};
        }))
        .def_static("parse", [](std::string const& str) { return rock::parse_board_position(str); })
        .def("__str__", [](rock::BoardPosition x) { return to_string(x); })
        .def("__repr__", [](rock::BoardPosition x) {
            return fmt::format("rock.BoardPosition('{}')", x);
        });

    pybind11::class_<rock::Move>(m, "Move")
        .def(pybind11::init<>())
        .def(pybind11::init([](std::string const& str) { return rock::parse_move(str).value(); }))
        .def(pybind11::init([](rock::BoardPosition from, rock::BoardPosition to) {
            return rock::Move{from, to};
        }))
        .def("is_legal", &rock::is_legal_move, "board"_a, "player"_a)
        .def_static("parse", [](std::string const& str) { return rock::parse_move(str); })
        .def("__str__", [](rock::Move x) { return to_string(x); })
        .def("__repr__", [](rock::Move x) { return fmt::format("rock.Move('{}')", x); });

    pybind11::class_<rock::Board>(m, "Board")
        .def(pybind11::init<>())
        .def("apply_move", &py_apply_move)
        .def("game_outcome", rock::get_game_outcome, "player_to_move"_a)
        .def("__format__",&py_format_board)
        .def("__str__", [](rock::Board const& x) { return to_string(x); });

    // Functions

    m.attr("starting_board") = rock::starting_board;

    m.def(
        "generate_moves",
        [](rock::Board const& b, rock::Player p) {
            auto const moves = rock::generate_moves(b, p);
            return std::vector(moves.begin(), moves.end());
        },
        "board"_a,
        "player"_a);

    m.def(
        "count_moves",
        &rock::count_moves,
        "board"_a = rock::starting_board,
        "player"_a = rock::Player::White,
        "level"_a = 1);

    m.def(
        "evaluate",
        [](rock::Board const& b, rock::Player p) {
            return rock::normalize_score(rock::evaluate_position(b, p), p);
        },
        "board"_a,
        "player"_a);

    m.def(
        "recommend_move",
        [](rock::Board const& b, rock::Player p) {
            auto [move, score] = rock::recommend_move(b, p);
            score = rock::normalize_score(score, p);
            return pybind11::make_tuple(move, score);
        },
        "board"_a,
        "player"_a);
}
