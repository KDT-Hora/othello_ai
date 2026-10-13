#include "ai_player.hpp"

#include <algorithm>
#include <limits>

namespace cubo_othello {

namespace {
int8_t opponent_of(int8_t color) { return (color == BLACK) ? WHITE : BLACK; }

// Stone-count swing from `to_move`'s own perspective when it places a stone
// that flips `flip_count` opponent stones: +1 for the placed stone, +flip_count
// converted to its color, -flip_count removed from the opponent -> 2f + 1.
int move_gain(std::size_t flip_count) { return 2 * static_cast<int>(flip_count) + 1; }

// Order moves by immediate gain (descending): cheap to compute, and making the
// strongest moves first maximises alpha-beta cutoffs.
void order_moves(std::vector<LegalMove>& moves) {
    std::sort(moves.begin(), moves.end(), [](const LegalMove& a, const LegalMove& b) {
        return a.flips.size() > b.flips.size();
    });
}
} // namespace

SimpleAI::SimpleAI(int8_t color, int depth)
    : color_(color), depth_(depth), rng_(std::random_device{}()) {}

int SimpleAI::evaluate(const CubeBoard& board) const {
    return board.count(color_) - board.count(opponent_of(color_));
}

int SimpleAI::minimax(CubeBoard& board, int8_t to_move, int ply_left, int alpha, int beta, int score) const {
    if (ply_left <= 0) return score;

    auto moves = board.legal_moves_with_flips(to_move);
    if (moves.empty()) {
        const int8_t opp = opponent_of(to_move);
        if (!board.has_valid_moves(opp)) return score; // neither side can move
        return minimax(board, opp, ply_left - 1, alpha, beta, score); // pass
    }

    order_moves(moves);

    const bool maximizing = (to_move == color_);
    int best = maximizing ? std::numeric_limits<int>::min() : std::numeric_limits<int>::max();

    for (const auto& lm : moves) {
        const int swing = move_gain(lm.flips.size());
        const int child_score = (to_move == color_) ? score + swing : score - swing;

        board.apply_move(lm.move.x, lm.move.y, lm.move.z, to_move);
        int value = minimax(board, opponent_of(to_move), ply_left - 1, alpha, beta, child_score);
        board.undo_move(lm.move, lm.flips, to_move);

        if (maximizing) {
            if (value > best) best = value;
            if (best > alpha) alpha = best;
        } else {
            if (value < best) best = value;
            if (best < beta) beta = best;
        }
        if (alpha >= beta) break; // cutoff
    }

    return best;
}

std::optional<Move> SimpleAI::choose_move(const CubeBoard& board) const {
    auto moves = board.legal_moves_with_flips(color_);
    if (moves.empty()) return std::nullopt;

    order_moves(moves);

    // One working copy of the board; the search makes/unmakes in place so no
    // node allocates or copies a board. The caller's `board` is untouched.
    CubeBoard work = board;
    const int root_score = evaluate(board);

    int best_score = std::numeric_limits<int>::min();
    std::vector<Move> best_moves;

    for (const auto& lm : moves) {
        const int swing = move_gain(lm.flips.size());

        // Scout window: alpha = best_score - 1, beta = +infinity. A child that
        // returns a value <= alpha is provably strictly worse than the current
        // best (fail-low bound) and is excluded; any child whose true value is
        // >= best_score exceeds alpha and is therefore searched to an exact
        // score (no fail-high is possible with beta = +infinity). The set of
        // tied-best moves - and its random tie-break - is identical to the
        // unpruned search, while weak root subtrees get pruned across roots.
        const int alpha = (best_score == std::numeric_limits<int>::min())
                              ? best_score
                              : best_score - 1;

        work.apply_move(lm.move.x, lm.move.y, lm.move.z, color_);
        int score = minimax(work, opponent_of(color_), depth_ - 1,
                            alpha, std::numeric_limits<int>::max(),
                            root_score + swing);
        work.undo_move(lm.move, lm.flips, color_);

        if (score > best_score) {
            best_score = score;
            best_moves = {lm.move};
        } else if (score == best_score) {
            best_moves.push_back(lm.move);
        }
    }

    std::uniform_int_distribution<size_t> dist(0, best_moves.size() - 1);
    return best_moves[dist(rng_)];
}

} // namespace cubo_othello
