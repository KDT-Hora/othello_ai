// GameEngine::run() -- the DXLib window loop. Kept separate from
// game_engine.cpp so the turn/undo logic there can be unit-tested without
// linking DXLib. Only built into cubo_othello.
#include "game_engine.hpp"

#include <algorithm>
#include <string>

#include <DxLib.h>

namespace cubo_othello {

namespace {
std::string result_message(const CubeBoard& board) {
    int b = board.count(BLACK);
    int w = board.count(WHITE);
    if (b > w) return "黒（シアン）の勝ち！  Rキーで再戦";
    if (w > b) return "白（マゼンタ）の勝ち！  Rキーで再戦";
    return "引き分け  Rキーで再戦";
}

// Search depth for the menu's difficulty levels (easy / normal / hard).
constexpr int kDifficultyDepth[kMenuDifficultyCount] = {1, 2, 3};

// AI vs AI: pause between moves so the game is watchable.
constexpr int kSpectateMoveIntervalMs = 700;
} // namespace

int GameEngine::run() {
    // FR-007's literal 120x80 default is too small for real mouse play;
    // scale up 8x while keeping the same projection math (resolution-
    // independent) and aspect ratio.
    if (!display_.init(960, 640)) return 1;

    // Menu selection: mode 0 = human vs human, 1 = human is Black (AI White),
    // 2 = human is White (AI Black), 3 = AI vs AI (spectate).
    int mode = 0;
    int difficulty = 1;
    bool in_menu = !skip_menu_; // "--ai" on the command line goes straight to the game
    bool quit = false;
    int last_ai_move_ms = 0;

    if (!in_menu) display_.set_ai_color(ai_ ? std::optional<int8_t>(ai_->color()) : std::nullopt);

    while (!quit) {
        if (in_menu) {
            display_.render_menu(mode, difficulty);
            const MenuInput in = display_.handle_menu_input();
            if (in.quit) break;

            if (in.move != 0) mode = std::clamp(mode + in.move, 0, kMenuModeCount - 1);
            if (in.diff_move != 0) difficulty = std::clamp(difficulty + in.diff_move, 0, kMenuDifficultyCount - 1);
            if (in.mode_row >= 0) mode = in.mode_row;
            if (in.difficulty_col >= 0) difficulty = in.difficulty_col;

            if (in.confirm) {
                std::optional<int8_t> ai_color;
                if (mode == 1) ai_color = WHITE;
                if (mode == 2) ai_color = BLACK;
                const bool spectate = (mode == 3);
                start_game(ai_color, kDifficultyDepth[difficulty], spectate);
                display_.set_ai_color(ai_color);
                display_.set_spectate(spectate);
                last_ai_move_ms = GetNowCount();
                in_menu = false;
            }
            continue;
        }

        const GameOverReason reason = board_.game_over_reason();
        const std::string message =
            (reason == GameOverReason::InProgress) ? std::string{} : result_message(board_);

        display_.render_frame(board_, turn_, message);
        InputResult input = display_.handle_input(board_, turn_);

        if (input.quit) break;

        if (input.menu_requested) {
            in_menu = true;
            continue;
        }

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

        SimpleAI* current_ai = nullptr;
        if (ai_ && ai_->color() == turn_) current_ai = &*ai_;
        else if (ai2_ && ai2_->color() == turn_) current_ai = &*ai2_;

        if (current_ai) {
            if (ai2_ && GetNowCount() - last_ai_move_ms < kSpectateMoveIntervalMs) continue;
            last_ai_move_ms = GetNowCount();
            auto move = current_ai->choose_move(board_);
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
