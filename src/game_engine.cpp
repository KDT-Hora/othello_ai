// Cube Othello — Game Engine (Terminal Fallback Mode)
#include <iostream>
#include "board.hpp"

namespace cubo {

int GameEngine::run() {
    std::cout << "\n╔══════════════════════════════╗\n";
    std::cout << "║   CUBE OTHELLO — Terminal    ║\n";
    std::cout << "╚══════════════════════════════╝\n\n";

    while (!game_over_) {
        render_board();

        if (check_game_over()) break;

        PieceColor turn = get_turn_player();  // 黒が先攻
        std::string turn_str = (turn == PieceColor::BLACK) ? "⚫ BLACK" : "⚪ WHITE";
        std::cout << "\n━━━ Turn: [" << turn_str << "] ━━━\n";

        auto valid_moves = board_.get_all_valid_moves();

        if (valid_moves.empty()) {
            std::cout << "No valid moves — switching turn...\n\n";
            turn_player_ = (turn_player_ == PieceColor::BLACK) ? PieceColor::WHITE : PieceColor::BLACK;
        } else {
            // 簡易：ランダムな有効手を選択（MVP: 最初の有効手を採用）
            auto& move = valid_moves[0];   // MVP: 最初の有効手を採用
            int mx = std::get<0>(move);
            int my = std::get<1>(move);

            board_.set_cell(mx, my, turn_player_);

            // 挟み込みによる石の反転
            auto flipped = board_.flip_stones(mx, my, turn_player_);

            if (!flipped.empty()) {
                int count = static_cast<int>(flipped.size());
                std::cout << "Flipped " << count << " stone(s)!\n";
            } else {
                std::cout << "(No sandwich — piece placed without flip)\n";
            }

            move_count_++;
        }
    }

    render_final_score();
    return 0;
}

void GameEngine::render_board() {
    board_.print();
}

bool GameEngine::check_game_over() {
    auto counts = board_.count_pieces();
    int black = std::get<0>(counts);
    int white = std::get<1>(counts);

    // 盤面が埋まっているか確認（簡易：隅のみで十分）
    bool all_filled = true;
    for (int x = 0; x < BOARD_SIZE; ++x) {
        if (board_.grid_[x][0] == PieceColor::EMPTY ||
            board_.grid_[x][BOARD_SIZE - 1] == PieceColor::EMPTY) {
            all_filled = false;
            break;
        }
    }

    if (all_filled && black + white == static_cast<int>(BOARD_SIZE * BOARD_SIZE)) {
        winner_ = 'D';   // ドロー（盤面埋まりきったが引き分け）
        game_over_ = true;
        return true;
    }

    // 有効手の有無でゲームオーバー判定
    auto valid_moves = board_.get_all_valid_moves();
    if (valid_moves.empty()) {
        winner_ = 'D';   // ドロー（stalemate）
        game_over_ = true;
        return true;
    }

    return false;
}

void GameEngine::render_final_score() const {
    auto counts = board_.count_pieces();
    int black_count = std::get<0>(counts);
    int white_count = std::get<1>(counts);

    std::cout << "\n━━━━━━━━━━━━━━━━━━\n";
    std::cout << "   FINAL SCORE      \n";
    std::cout << "  BLACK:  " << black_count << " stones\n";
    std::cout << "  WHITE:  " << white_count << " stones\n";

    if (black_count > white_count) {
        std::cout << "\n   Result: BLACK WINS!\n";
    } else if (white_count > black_count) {
        std::cout << "\n   Result: WHITE WINS!\n";
    } else {
        std::cout << "\n   Result: DRAW\n";
    }

    std::cout << "━━━━━━━━━━━━━━━━━━\n\n";
}

const CubeBoard& GameEngine::get_board() const { return board_; }

} // namespace cubo
