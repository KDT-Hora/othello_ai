// State/turn logic only -- no DXLib symbols referenced, so this file can be
// linked into cubo_tests without a DXLib install. GameEngine::run() (the
// actual DXLib window loop) lives in game_loop.cpp instead.
#include "game_engine.hpp"

namespace cubo_othello {

namespace {
int8_t opponent_of(int8_t color) { return (color == BLACK) ? WHITE : BLACK; }
} // namespace

GameEngine::GameEngine(std::optional<int8_t> ai_color, int ai_depth) {
    if (ai_color) ai_.emplace(*ai_color, ai_depth);
}

void GameEngine::advance_turn() {
    const int8_t opponent = opponent_of(turn_);
    if (board_.has_valid_moves(opponent)) {
        turn_ = opponent;
    }
    // else: opponent has no move, so `turn_` keeps its turn (a pass).
    // If neither side can move, game_over_reason() will report NoMoves.
}

void GameEngine::push_history() { history_.emplace_back(board_, turn_); }

void GameEngine::undo() {
    if (history_.empty()) return;

    board_ = history_.back().first;
    turn_ = history_.back().second;
    history_.pop_back();

    // If undoing a single move would hand control straight back to the AI,
    // undo one more step so the human regains control immediately instead
    // of watching the AI simply redo the same reply.
    if (ai_ && ai_->color() == turn_ && !history_.empty()) {
        board_ = history_.back().first;
        turn_ = history_.back().second;
        history_.pop_back();
    }
}

} // namespace cubo_othello
