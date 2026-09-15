#include "game_engine.hpp"

#include <string>

namespace cubo_othello {

namespace {
int8_t opponent_of(int8_t color) { return (color == BLACK) ? WHITE : BLACK; }

std::string result_message(const CubeBoard& board) {
    int b = board.count(BLACK);
    int w = board.count(WHITE);
    if (b > w) return "BLACK WINS! (R to restart)";
    if (w > b) return "WHITE WINS! (R to restart)";
    return "DRAW (R to restart)";
}
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
            continue;
        }

        if (reason != GameOverReason::InProgress) continue;

        if (ai_ && ai_->color() == turn_) {
            auto move = ai_->choose_move(board_);
            if (move) board_.place_stone(move->x, move->y, move->z, turn_);
            advance_turn();
        } else if (input.clicked_move) {
            const Move& mv = *input.clicked_move;
            board_.place_stone(mv.x, mv.y, mv.z, turn_);
            advance_turn();
        }
    }

    display_.shutdown();
    return 0;
}

} // namespace cubo_othello
