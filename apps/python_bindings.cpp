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
    auto const color = rock::BitBoard{x[rock::Player::White]}.at(m.from)
        ? rock::Player::White
        : rock::Player::Black;

    x = rock::apply_move(m, x, color);
}

auto py_pick_random_move(rock::Board const& board, rock::Player player_to_move) -> rock::Move
{
    static auto rng = std::mt19937{std::random_device{}()};
    return rock::pick_random_move(board, player_to_move, rng);
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
            return rock::Move{from.data(), to.data()};
        }))
        .def_static("parse", [](std::string const& str) { return rock::parse_move(str); })
        .def("__str__", [](rock::Move x) { return to_string(x); })
        .def("__repr__", [](rock::Move x) { return fmt::format("rock.Move('{}')", x); });

    pybind11::class_<rock::Board>(m, "Board")
        .def(pybind11::init<>())
        .def("apply_move", &py_apply_move)
        .def(
            "winning_player",
            [](rock::Board const& b) -> std::optional<rock::Player> {
                return rock::are_pieces_all_together(b[rock::Player::White])
                    ? std::optional{rock::Player::White}
                    : rock::are_pieces_all_together(b[rock::Player::Black])
                        ? std::optional{rock::Player::Black}
                        : std::nullopt;
            })
        .def("__str__", [](rock::Board const& x) { return to_string(x); });

    // Functions

    m.def("make_starting_board", [] { return rock::starting_board; });

    m.def("is_valid_move", [](rock::Move m, rock::Board const& b, rock::Player p) {
        auto const all_moves = rock::generate_moves(b, p);
        return std::find(all_moves.begin(), all_moves.end(), m) != all_moves.end();
    });

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

    m.def("pick_random_move", &py_pick_random_move, "board"_a, "player"_a);

    m.def("evaluate", &rock::evaluate_position_minmax, "board"_a, "player"_a, "depth"_a = 4);
    m.def("recommend_move", &rock::recommend_move, "board"_a, "player"_a);
}
