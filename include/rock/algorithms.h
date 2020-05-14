#pragma once

#include "types.h"
#include <functional>
#include <map>
#include <memory>
#include <vector>

/**
 * Note that none of the code in this module is thread-safe yet. There is a
 * global transposition table being accessed. The plan is that this will be
 * changed, and not only will the code be thread-safe (able to run multiple
 * analyses simultaneously), but the GameAnalyzer will support being controlled
 * from a separate thread.
 *
 * Ideally, there will also be support for configuring the execution policy of
 * the analysis to be multi-threaded itself.
 */

namespace rock
{

ROCK_API auto apply_move(Move, Board, Player) -> Board;
ROCK_API auto apply_move(Move, Position) -> Position;

ROCK_API auto list_moves(Position const&) -> std::vector<Move>;
ROCK_API auto count_moves(Position const&, int level = 1) -> std::size_t;
ROCK_API auto is_move_legal(Move, Position const&) -> bool;
ROCK_API auto list_legal_destinations(BoardCoordinates from, Position const&)
    -> std::vector<BoardCoordinates>;

ROCK_API auto get_game_outcome(Position const&) -> GameOutcome;

/**
 * Analyze a position up to a fixed depth
 */
ROCK_API auto analyze_position(Position const&, int depth) -> PositionAnalysis;

/**
 * Separately analyze each available move
 *
 * This will probably take considerably longer than only analyzing the root
 * node, as an accurate score will be determined for each move.
 */
ROCK_API auto analyze_available_moves(Position const&, int depth) -> std::map<Move, PositionAnalysis>;

/**
 * Create a position analysis based on the input using a soft max function to
 * determine which move to select
 *
 * This can be controlled through the parameter - 0.0 will return a random move,
 * inf will always return the best move.
 */
ROCK_API auto select_analysis_with_softmax(
    std::map<Move, PositionAnalysis> const&, double softmax_parameter = 1.0) -> PositionAnalysis;

/**
 * Analyze the position but vary the depth/softmax_parameter/other with the
 * supplied ai difficulty level
 *
 * The ai_level input can range from 0 -> 10+, and the quality of the results
 * will increase. Beware that higher difficulty levels (above 10) will begin to
 * take longer to execute.
 */
ROCK_API auto analyze_position_with_ai_difficulty_level(Position const&, int ai_level)
    -> PositionAnalysis;

/**
 * Crude initial attempt at an object through which the analysis of a position
 * can be controlled.
 */
struct ROCK_API GameAnalyzer
{
    explicit GameAnalyzer();

    auto analyze_position(Position) -> void;
    auto stop_analysis() -> void;
    auto is_analysis_ongoing() const -> bool;

    auto set_max_depth(int) -> void;
    auto set_report_callback(std::function<void(GameAnalyzer&)>) -> void;
    auto best_analysis_so_far() -> PositionAnalysis;
    auto current_depth() const -> int;

    ~GameAnalyzer();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace rock
