// Cube Othello -- SimpleAI: depth-limited minimax with alpha-beta pruning,
// flip-count move ordering, incremental stone-count evaluation, and in-place
// make/unmake on a single working board (no per-node board copies).
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
    // (i.e. it must pass). Never returns an illegal move. The board passed in
    // is never mutated. Root children are searched with a scout window
    // (alpha = best_so_far - 1, beta = +inf): ties and better moves always
    // receive an exact score, strictly-worse moves may be pruned, so the set
    // of tied-best moves and its random tie-break match the unpruned search.
    std::optional<Move> choose_move(const CubeBoard& board) const;

private:
    int8_t color_;
    int depth_;
    mutable std::mt19937 rng_;

    // Minimax with alpha-beta pruning on `board`, mutated in place via
    // apply_move/undo_move. `to_move` is the color to play at this node;
    // `score` is the stone-count difference (from color_'s perspective) of the
    // current board, maintained incrementally instead of rescanned at leaves.
    // The returned score is always from color_'s perspective.
    int minimax(CubeBoard& board, int8_t to_move, int ply_left, int alpha, int beta, int score) const;
};

} // namespace cubo_othello
