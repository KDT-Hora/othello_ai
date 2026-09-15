// Cube Othello -- SimpleAI: depth-limited minimax with no pruning.
#pragma once

#include <cstdint>
#include <optional>
#include <random>

#include "board.hpp"

namespace cubo_othello {

class SimpleAI {
public:
    explicit SimpleAI(int8_t color, int depth = 3);

    int8_t color() const { return color_; }

    // Stone-count evaluation from this AI's own perspective:
    // positive = favorable to `color()`.
    int evaluate(const CubeBoard& board) const;

    // Returns the chosen move, or nullopt if `color()` has no legal move
    // (i.e. it must pass). Never returns an illegal move.
    std::optional<Move> choose_move(const CubeBoard& board) const;

private:
    int8_t color_;
    int depth_;
    mutable std::mt19937 rng_;

    // Minimax without alpha-beta pruning. `to_move` is the color to play at
    // this node; the returned score is always from color_'s perspective.
    int minimax(CubeBoard board, int8_t to_move, int ply_left) const;
};

} // namespace cubo_othello
