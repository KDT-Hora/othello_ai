// GameEngine::run() -- the DXLib window loop. Kept separate from
// game_engine.cpp so the turn/undo logic there can be unit-tested without
// linking DXLib. Only built into cubo_othello.
#include "game_engine.hpp"

#include <string>

namespace cubo_othello {

namespace {
std::string result_message(const CubeBoard& board) {
    int b = board.count(BLACK);
    int w = board.count(WHITE);
    if (b > w) return "黒（シアン）の勝ち！  Rキーで再戦";
    if (w > b) return "白（マゼンタ）の勝ち！  Rキーで再戦";
    return "引き分け  Rキーで再戦";
}
} // namespace

int GameEngine::run() {
    // FR-007's literal 120x80 default is too small for real mouse play;
    // scale up 8x while keeping the same projection math (resolution-
    // independent) and aspect ratio.
    if (!display_.init(960, 640)) return 1;

    bool quit = false;

    while (!quit) {
        const GameOverReason reason = board_.game_over_reason();
        const std::string message =
            (reason == GameOverReason::InProgress) ? std::string{} : result_message(board_);

        display_.render_frame(board_, turn_, message);
        InputResult input = display_.handle_input(board_, turn_);

        if (input.quit) break;

        if (input.reset_requested) {
            board_.initialize();
            turn_ = BLACK;
            history_.clear();
            continue;
        }

        if (input.undo_requested) {
            undo();
            continue;
        }

        if (reason != GameOverReason::InProgress) continue;

        if (ai_ && ai_->color() == turn_) {
            auto move = ai_->choose_move(board_);
            if (move) {
                push_history();
                board_.place_stone(move->x, move->y, move->z, turn_);
                advance_turn();
            }
        } else if (input.clicked_move) {
            const Move& mv = *input.clicked_move;
            push_history();
            board_.place_stone(mv.x, mv.y, mv.z, turn_);
            advance_turn();
        }
    }

    display_.shutdown();
    return 0;
}

} // namespace cubo_othello
