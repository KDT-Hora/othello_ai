// Cube Othello -- GameEngine: turn management, pass rules, and the main loop.
#pragma once

#include <optional>
#include <utility>
#include <vector>

#include "ai_player.hpp"
#include "board.hpp"
#include "display.hpp"

namespace cubo_othello {

// Test-only accessor: lets tests exercise turn/undo logic (advance_turn,
// push_history, undo) without needing a real DXLib window.
struct GameEngineTestAccess;

class GameEngine {
public:
    // ai_color: BLACK, WHITE, or std::nullopt for human-vs-human.
    explicit GameEngine(std::optional<int8_t> ai_color = std::nullopt, int ai_depth = 3);

    // Runs the DXLib window loop until the user quits. Returns an exit code.
    int run();

    // Resets to a fresh game: `ai_color` = nullopt for human vs human.
    // `spectate` = AI vs AI (ai_color is ignored); both AIs use `ai_depth`.
    void start_game(std::optional<int8_t> ai_color, int ai_depth, bool spectate = false);

private:
    CubeBoard board_;
    int8_t turn_ = BLACK;
    DXLibDisplay display_;
    std::optional<SimpleAI> ai_;
    std::optional<SimpleAI> ai2_; // second AI, only in AI-vs-AI (ai_ = Black, ai2_ = White)
    bool skip_menu_ = false; // command line already chose the mode

    // Snapshots taken before each placed stone (T035, optional undo).
    std::vector<std::pair<CubeBoard, int8_t>> history_;

    // Applies FR-004: switch to the opponent, or pass back if the opponent
    // also has no legal moves (game-over is then detected by the caller).
    void advance_turn();

    // Records the current state so a later placement can be undone.
    void push_history();

    // Restores the most recent snapshot, if any (no-op when history_ is empty).
    void undo();

    friend struct GameEngineTestAccess;
};

} // namespace cubo_othello
