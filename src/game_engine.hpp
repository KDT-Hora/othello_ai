// Cube Othello -- GameEngine: turn management, pass rules, and the main loop.
#pragma once

#include <optional>

#include "ai_player.hpp"
#include "board.hpp"
#include "display.hpp"

namespace cubo_othello {

class GameEngine {
public:
    // ai_color: BLACK, WHITE, or std::nullopt for human-vs-human.
    explicit GameEngine(std::optional<int8_t> ai_color = std::nullopt, int ai_depth = 3);

    // Runs the DXLib window loop until the user quits. Returns an exit code.
    int run();

private:
    CubeBoard board_;
    int8_t turn_ = BLACK;
    DXLibDisplay display_;
    std::optional<SimpleAI> ai_;

    // Applies FR-004: switch to the opponent, or pass back if the opponent
    // also has no legal moves (game-over is then detected by the caller).
    void advance_turn();
};

} // namespace cubo_othello
