// Cube Othello — Main Entry Point (C++ / DxLib)
// ============================================================================
// メインプログラム：引数処理、ゲームループ制御、フォールバック描画（ターミナル絵文字）

#include <iostream>
#include <string>
#include <cstdlib>
#include "board.hpp"
#include "game.hpp"
#include "gui.hpp"   // DxLib 版 GUI（DxLib が未リンクならフォールバックモードへ切り替わる）

using namespace cubo;

// ============================================================================
void print_usage() {
    std::cout << R"(
Cube Othello — C++ / DxLib Implementation
Usage:   cubo_othello [--terminal] [--ai-depth <n>] [-v]

Options:
  --terminal   Use terminal emoji rendering instead of DxLib (default)
  --ai-depth <n>   Set AI search depth (1-5, default=3)
  -v           Verbose output with game state

Example: cubo_othello --terminal --ai-depth 3 -v
)";
}

// ============================================================================
int main(int argc, char** argv) {
    bool terminal_mode = false;
    int ai_depth = 3;   // SimpleAI の深さ制限（簡易版：石数評価のみ）

    // 引数の解析（簡易パース）
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--terminal") == 0 || std::strcmp(argv[i], "-t") == 0) {
            terminal_mode = true;
        } else if (std::strcmp(argv[i], "--ai-depth") == 0 && i + 1 < argc) {
            ai_depth = std::atoi(argv[++i]);
            if (ai_depth < 1) ai_depth = 1;
            if (ai_depth > 5) ai_depth = 5;
        } else if (std::strcmp(argv[i], "-v") == 0 || std::strcmp(argv[i], "--verbose") == 0) {
            // verbose フラグの処理（簡易）：詳細ログを出力
        } else if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
            print_usage();
            return EXIT_SUCCESS;
        }
    }

    // 初期化とゲームループ
    GameEngine engine{};
    CubeGridRenderer renderer(engine.get_board());

    std::cout << "=== Cube Othello ===" << std::endl;
    std::cout << "Render mode: " << (terminal_mode ? "Terminal (emoji)" : "DxLib") << std::endl;
    std::cout << "AI depth:    " << ai_depth << std::endl;
    std::cout << std::endl;

    // ゲームループ（簡易：ターミナルフォールバックモード）
    while (!engine.is_game_over()) {
        const auto& board = engine.get_board();

        // 盤面描画（ターミナルフォールバック）
        for (int z = BOARD_SIZE - 1; z >= 0; --z) {
            std::cout << "[";
            for (int x = 0; x < BOARD_SIZE; ++x) {
                char ch;
                switch (board.get_cell(x, y, z)) {
                    case '.': ch = '.'; break;
                    case 'B': ch = '⚫'; break;
                    case 'W': ch = '⚪'; break;
                }
                std::cout << ch;
            }
            std::cout << "] (z=" << z << ")" << std::endl;
        }

        // 有効な手の表示（簡易）
        auto valid_moves = engine.get_valid_moves_for_current_player();
        if (!valid_moves.empty()) {
            std::cout << "Valid moves: (" << valid_moves[0][0] << ","
                      << valid_moves[0][1] << "," << valid_moves[0][2] << ") ..." << std::endl;
        }

        // 簡易入力シミュレーション（コマンドラインエコー）
        char input[64];
        if (terminal_mode) {
            printf("Enter move (x y z): ");
            std::cin.getline(input, sizeof(input));
        } else {
            printf("[AI turn] Searching...\n");
            // AI の簡易選択（石数差最大化）
            auto moves = engine.get_valid_moves_for_current_player();
            if (!moves.empty()) {
                int best_x = moves[0][0], best_y = moves[1][0];  // シンプル：最初の有効手
                int best_z = moves[2][0];

                auto result = engine.play_move(best_x, best_y, best_z);
                printf("AI placed at (%d,%d,%d), flipped %d stones\n",
                       best_x, best_y, best_z, result.flipped_count);
            } else {
                std::cout << "No valid moves for AI. Passing turn.\n";
            }
        }

        engine.check_game_over();
    }

    // ゲーム終了時の表示
    auto winner = engine.get_winner_name();
    auto counts = engine.piece_counts;
    std::cout << "\n=== Game Over ===" << std::endl;
    std::cout << "Winner: " << (winner == 'B' ? "BLACK" : winner == 'W' ? "WHITE" : "DRAW") << std::endl;
    std::cout << "Score:  Black=" << counts['B'] << ", White=" << counts['W'] << std::endl;

    return EXIT_SUCCESS;
}
