// Cube Othello — Main Entry Point (C++17 / Terminal Fallback Mode)
#include <iostream>
#include <string>
#include "board.hpp"
#include "game_engine.hpp"

int main(int argc, char** argv) {
    std::cout << "╔════════════════════════════╗\n";
    std::cout << "║   CUBE OTHELLO (v0.1)     ║\n";
    std::cout << "║   8x8x3 — Terminal Mode   ║\n";
    std::cout << "╚════════════════════════════╝\n\n";

    // 引数処理：--ai フラグで AI モード（簡易）
    bool ai_mode = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--ai") {
            ai_mode = true;
        }
    }

    // ゲームエンジン作成
    cubo::GameEngine game;

    if (ai_mode) {
        std::cout << "  AI Mode: ON\n";
    } else {
        std::cout << "  AI Mode: OFF (Human vs Human)\n";
    }

    // ゲーム開始
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  Game Start!\n";
    std::cout << "  Place stones at coordinates: x,y\n";
    std::cout << "  Example: place stone at B4 → 0,3\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━\n\n";

    // ゲームループ実行
    int result = game.run();

    if (result == 1) {
        std::cout << "  [AI Mode] AI defeated human player!\n";
    } else if (result == 2) {
        std::cout << "  Game ended: no valid moves for either side.\n";
    } else if (result == 3) {
        std::cout << "  Draw game (board filled).\n";
    }

    return 0;   // Always exit cleanly
}
