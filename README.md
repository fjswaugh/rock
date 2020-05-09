Rock
====

This library (`rock`) is a C++17 AI for playing games of the checkers-like
board game I know only as 'Romanian Checkers'. The library provides functions
for parsing and formatting game positions and moves, as well as analysing
positions and managing games.

Building the library
--------------------

Requires CMake version 3.14 or greater, and a C++17 capable compiler. To build
(`-G Ninja` is optional):

`cmake -B build/release -S . -G Ninja -DCMAKE_BUILD_TYPE=Release`

...and then...

`cmake --build build`

The library can be added as a CMake submodule and included by linking against
the CMake `rock` target.

### Dependencies

Right now dependencies are downloaded by CMake in the configure stage. This is not an ideal situation, but it works for now. Currently they consist of:
  - `fmt` (for formatting of `rock` types)
  - `doctest` (for unit testing)
  - `abseil` (currently used only for its hashing function)

### Options

Tests will be built by default if the library is the main CMake project being
built. The library can be built as either a static or dynamic library. Using
the `popcnt` instruction if available will result in a significant code speedup
(this can be enabled by turning on architecture-specific compiler
optimizations, which may be enabled under the CMake option
`ROCK_ARCHITECTURE_OPT`).

Rules of the game
-----------------

### Starting a game

The game is played on a standard chess/checkers board, 8 by 8 squares. Each
player starts with 12 pieces each arranged as follows (`W` represents white
pieces, `B` black). White begins with the first move, then the turns alternate.

```
   +-------------------------------+ 
 8 |   | W | W | W | W | W | W |   | 
   |---+---+---+---+---+---+---+---| 
 7 | B |   |   |   |   |   |   | B | 
   |---+---+---+---+---+---+---+---| 
 6 | B |   |   |   |   |   |   | B | 
   |---+---+---+---+---+---+---+---| 
 5 | B |   |   |   |   |   |   | B | 
   |---+---+---+---+---+---+---+---| 
 4 | B |   |   |   |   |   |   | B | 
   |---+---+---+---+---+---+---+---| 
 3 | B |   |   |   |   |   |   | B | 
   |---+---+---+---+---+---+---+---| 
 2 | B |   |   |   |   |   |   | B | 
   |---+---+---+---+---+---+---+---| 
 1 |   | W | W | W | W | W | W |   | 
   +-------------------------------+ 
     A   B   C   D   E   F   G   H   
```

### Making moves

The goal is to move all your pieces such that they are all touching. In the
following example, it is white to move. A player may move any of their pieces.
Consider moving the highlighted piece at `G6`.

```
   +-------------------------------+ 
 8 |   |   |   |   |   |   |   |   | 
   |---+---+---+---+---+---+---+---| 
 7 |   |   |   | B |   | B |   | B | 
   |---+---+---+---+---+---+---+---| 
 6 | B |   |   | B |   |   |*W*|   | <- Piece at G6
   |---+---+---+---+---+---+---+---| 
 5 |   |   |   |   |   |   |   |   | 
   |---+---+---+---+---+---+---+---| 
 4 |   |   |   |   |   |   | W |   | 
   |---+---+---+---+---+---+---+---| 
 3 | W |   |   | W |   | W |   |   | 
   |---+---+---+---+---+---+---+---| 
 2 | W |   |   |   | W |   |   |   | 
   |---+---+---+---+---+---+---+---| 
 1 |   | W |   |   |   |   |   |   | 
   +-------------------------------+ 
     A   B   C   D   E   F   G   H  
```

You can move in any direction: horizontal, vertical, or either diagonal. You
can move by a number of squares equal to the number of pieces of either color
that lie on that line.

Certain squares on the graphic below are highlighted, `* *` represents viable
destinations for the piece on `G6`:
  - `G8` is 2 squares away, and since there are 2 pieces on the vertical line
    it is a viable destination
  - `E6` is 3 squares away, if chosen the enemy (`B`) piece would be taken and
    removed from the game
  - `C2` is 4 squares away (4 pieces on that diagonal including the enemy
    piece), and is viable despite there being a friendly piece in the way

`> <` represents destinations that would be viable based on count, but are not
for some reason:
  - The `E8` square would be viable were the line not obstructed by an enemy
    piece
    - You can jump over your own pieces but not enemy pieces
  - The `G4` square would be viable except you cannot land on your own piece
    - You can land and take enemy pieces but not your own

```
   +-------------------------------+ 
 8 |   |   |   |   |> <|   |* *|   | 
   |---+---+---+---+---+---+---+---| 
 7 |   |   |   | B |   | B |   | B | 
   |---+---+---+---+---+---+---+---| 
 6 | B |   |   |*B*|   |   | W |   | 
   |---+---+---+---+---+---+---+---| 
 5 |   |   |   |   |   |   |   |   | 
   |---+---+---+---+---+---+---+---| 
 4 |   |   |   |   |   |   |>W<|   | 
   |---+---+---+---+---+---+---+---| 
 3 | W |   |   | W |   | W |   |   | 
   |---+---+---+---+---+---+---+---| 
 2 | W |   |* *|   | W |   |   |   | 
   |---+---+---+---+---+---+---+---| 
 1 |   | W |   |   |   |   |   |   | 
   +-------------------------------+ 
     A   B   C   D   E   F   G   H   
```

### Victory condition

To win, you must 'connect' all your pieces - that is, they must all be
touching. In the above example, if white moves their piece from `G6` to `C2`
all the white pieces would be touching and white would win the game.

If a player is left with only one piece they automatically win the game. If a
player connects all their pieces with a move that simultaneously causes the
opponent to win (e.g. by taking an unconnected opponent piece) the game is a
draw.

If a player has no legal moves available then the game is a draw, however if a
move causes you to win but would also mean the opponent would have no moves
available, then you still win the game.
