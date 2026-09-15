#include "ai_player.hpp"

#include <algorithm>
#include <limits>

namespace cubo_othello {

namespace {
int8_t opponent_of(int8_t color) { return (color == BLACK) ? WHITE : BLACK; }
} // namespace

SimpleAI::SimpleAI(int8_t color, int depth)
    : color_(color), depth_(depth), rng_(std::random_device{}()) {}

int SimpleAI::evaluate(const CubeBoard& board) const {
    return board.count(color_) - board.count(opponent_of(color_));
}

int SimpleAI::minimax(CubeBoard board, int8_t to_move, int ply_left) const {
    if (ply_left <= 0) return evaluate(board);

    auto moves = board.valid_moves(to_move);
    if (moves.empty()) {
        const int8_t opp = opponent_of(to_move);
        if (!board.has_valid_moves(opp)) return evaluate(board); // neither side can move
        return minimax(board, opp, ply_left - 1);                // pass
    }

    const bool maximizing = (to_move == color_);
    int best = maximizing ? std::numeric_limits<int>::min() : std::numeric_limits<int>::max();

    for (const auto& mv : moves) {
        CubeBoard next = board;
        next.place_stone(mv.x, mv.y, mv.z, to_move);
        int score = minimax(next, opponent_of(to_move), ply_left - 1);
        best = maximizing ? std::max(best, score) : std::min(best, score);
    }

    return best;
}

std::optional<Move> SimpleAI::choose_move(const CubeBoard& board) const {
    auto moves = board.valid_moves(color_);
    if (moves.empty()) return std::nullopt;

    int best_score = std::numeric_limits<int>::min();
    std::vector<Move> best_moves;

    for (const auto& mv : moves) {
        CubeBoard next = board;
        next.place_stone(mv.x, mv.y, mv.z, color_);
        int score = minimax(next, opponent_of(color_), depth_ - 1);

        if (score > best_score) {
            best_score = score;
            best_moves = {mv};
        } else if (score == best_score) {
            best_moves.push_back(mv);
        }
    }

    std::uniform_int_distribution<size_t> dist(0, best_moves.size() - 1);
    return best_moves[dist(rng_)];
}

} // namespace cubo_othello
