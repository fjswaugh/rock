#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <rock/algorithms.h>
#include <rock/format.h>
#include <rock/starting_board.h>

using namespace pybind11::literals;

namespace
{

auto py_apply_move(rock::Board& x, rock::Move m) -> void
{
    auto const color = rock::BitBoard{x.pieces[bool(rock::Color::White)]}.at(m.from)
        ? rock::Color::White
        : rock::Color::Black;

    x = rock::apply_move(m, x, color);
}

auto py_pick_random_move(rock::Board const& board, rock::Color player_to_move) -> rock::Move
{
    static auto rng = std::mt19937{std::random_device{}()};
    return rock::pick_random_move(board, player_to_move, rng);
}

}  // namespace

PYBIND11_MODULE(rock, m)
{
    m.doc() = "Rock library for Romanian Checkers";

    // Types

    pybind11::enum_<rock::Color>(m, "Color")
        .value("White", rock::Color::White)
        .value("Black", rock::Color::Black)
        .export_values();

    pybind11::class_<rock::BoardPosition>(m, "BoardPosition")
        .def(pybind11::init<>())
        .def("__str__", [](rock::BoardPosition x) { return to_string(x); });

    pybind11::class_<rock::Move>(m, "Move")
        .def(pybind11::init<>())
        .def("__str__", [](rock::Move x) { return to_string(x); });

    pybind11::class_<rock::Board>(m, "Board")
        .def(pybind11::init<>())
        .def("apply_move", &py_apply_move)
        .def("__str__", [](rock::Board const& x) { return to_string(x); });

    // Functions

    m.def("make_starting_board", [] { return rock::starting_board; });

    m.def(
        "generate_moves",
        [](rock::Board const& b, rock::Color p) {
            auto const moves = rock::generate_moves(b, p);
            return std::vector(moves.begin(), moves.end());
        },
        "board"_a,
        "player"_a);

    m.def(
        "count_moves",
        &rock::count_moves,
        "board"_a = rock::starting_board,
        "player"_a = rock::Color::White,
        "level"_a = 1);

    m.def("pick_random_move", &py_pick_random_move, "board"_a, "player"_a);
}
