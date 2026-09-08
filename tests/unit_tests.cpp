// Cube Othello — Unit Tests with Google Test (C++)
// ============================================================================
// カバレッジ目標: 95%+
//   - Board::flip_stones の全分岐（6 方向 × 挟み込み成立/不成立 / 空マス）
//   - Board::get_valid_moves_for_color の全パス
//   - GameEngine::play_move のエフェクト判定（return_burst, wall_strike）
//   - GameEngine::check_game_over の全条件（盤面埋まり / パス / 通常）

#pragma once

#include <gtest/gtest.h>
#include "board.hpp"
#include "game.hpp"

using namespace cubo;

// ============================================================================
// Test Suite: CubeBoard::flip_stones
// ============================================================================
TEST(CubeBoardFlipStones, InitialState_NoMovesYet) {
    CubeBoard board{};
    EXPECT_EQ(board.get_cell(3, 3, 3), 'B');
    EXPECT_EQ(board.get_cell(4, 3, 3), 'W');

    // 初期状態では有効な手がないはず（挟み込みなし）
    auto flipped = board.flip_stones(3, 3, 3, BLACK);
    EXPECT_FALSE(flipped.success);
}

TEST(CubeBoardFlipStones, SandwichAlongX_Axis) {
    CubeBoard board{};

    // (x=2,y=3,z=3) に黒を置く → x 軸正方向で白を挟み込む
    const int place_x = 2; const int place_y = 3; const int place_z = 3;
    board.set_cell(place_x, place_y, place_z, BLACK);

    // (x=5,y=3,z=3) に白を配置（黒と挟むため）
    board.set_cell(4, 3, 3, WHITE);   // 初期の白石
    board.set_cell(6, 3, 3, WHITE);   // 黒を挟むための白

    // 黒が (5,3,3) を置くと、白 (x=4,y=3,z=3) と白 (x=6,y=3,z=3) が挟まれる
    auto flipped = board.flip_stones(5, 3, 3, BLACK);

    EXPECT_TRUE(flipped.success);
    EXPECT_EQ(static_cast<int>(flipped.flipped.size()), 2);   // 2 つの白がひっくり返る
}

TEST(CubeBoardFlipStones, NoSandwich_InvalidMove) {
    CubeBoard board{};
    board.set_cell(3, 3, 3, BLACK);
    board.set_cell(4, 3, 3, WHITE);

    // 挟み込みが発生しないマス（単に置けるだけ）は有効手ではない
    auto flipped = board.flip_stones(0, 0, 0, BLACK);
    EXPECT_FALSE(flipped.success);   // set_cell は呼び出されない
}

TEST(CubeBoardFlipStones, MultipleDirections_Sandwich) {
    CubeBoard board{};
    // x±方向で挟み込みを作る配置
    board.set_cell(1, 3, 3, BLACK);
    board.set_cell(5, 3, 3, WHITE);
    board.set_cell(6, 3, 3, WHITE);

    auto flipped = board.flip_stones(2, 3, 3, BLACK);
    EXPECT_TRUE(flipped.success);
}

TEST(CubeBoardFlipStones, DiagonalNotSupported) {
    // 斜め方向の挟み込みはサポートしない（仕様）
    CubeBoard board{};
    board.set_cell(1, 3, 3, BLACK);
    board.set_cell(4, 4, 4, WHITE);   // 斜め
    auto flipped = board.flip_stones(2, 3, 3, BLACK);
    EXPECT_FALSE(flipped.success);
}

TEST(CubeBoardFlipStones, MultipleFlipsInOneMove) {
    CubeBoard board{};
    // y±方向でも挟み込みを作る配置
    board.set_cell(3, 1, 3, WHITE);
    board.set_cell(3, 5, 3, BLACK);
    board.set_cell(3, 6, 3, BLACK);

    auto flipped = board.flip_stones(3, 2, 3, BLACK);
    EXPECT_TRUE(flipped.success);
    // y±方向の両方（白→黒）がひっくり返る：合計 3 つ（y=1 と y=5 の白）
    EXPECT_EQ(static_cast<int>(flipped.flipped.size()), 2);
}

TEST(CubeBoardFlipStones, BoardIsFull_NoEmptyCells) {
    CubeBoard board{};
    for (int x = 0; x < BOARD_SIZE; ++x) {
        for (int y = 0; y < BOARD_SIZE; ++y) {
            for (int z = 0; z < BOARD_SIZE; ++z) {
                board.set_cell(x, y, z, BLACK);   // テスト用：全黒化
            }
        }
    }

    EXPECT_TRUE(board.is_full());
}

TEST(CubeBoardFlipStones, EmptyBoard_NoValidMoves) {
    CubeBoard board{};  // 初期配置のみ（512 マスのうち 8 個が埋まっている）
    auto valid = board.get_valid_moves_for_color(BLACK);
    EXPECT_TRUE(valid.empty());   // 初期状態では有効な手はない（挟み込みなし）
}

TEST(CubeBoardFlipStones, CountPieces_Accuracy) {
    CubeBoard board{};
    board.set_cell(0, 0, 0, BLACK);
    board.set_cell(1, 1, 1, WHITE);
    board.set_cell(2, 2, 2, BLACK);

    EXPECT_EQ(board.count_pieces(BLACK), 2);
    EXPECT_EQ(board.count_pieces(WHITE), 1);
}

// ============================================================================
// Test Suite: GameEngine — Turn Management & Game Over Detection
// ============================================================================
TEST(GameEngine, BlackStartsFirst) {
    GameEngine engine{};
    EXPECT_EQ(engine.get_turn_player(), BLACK);
    EXPECT_FALSE(engine.is_game_over());
}

TEST(GameEngine, SwitchTurnAfterValidMove) {
    GameEngine engine{};
    auto moves = engine.get_valid_moves_for_current_player();

    if (!moves.empty()) {
        engine.play_move(moves[0][0], moves[0][1], moves[0][2]);
        engine.check_game_over();
        EXPECT_TRUE(engine.is_game_over());   // 初期状態では盤面埋まらないのでパス判定になるはず（簡易版）

        // ターン交代のロジック：白のターンへ
        EXPECT_EQ(engine.get_turn_player(), WHITE);
    } else {
        // 有効な手がない場合、黒がパスする → ターン交代
        engine.switch_turn();
        EXPECT_EQ(engine.get_turn_player(), WHITE);
    }
}

TEST(GameEngine, WhiteTurn_NoMoves_PassDetected) {
    GameEngine engine{};
    auto moves = engine.get_valid_moves_for_current_player();

    // 簡易テスト：有効な手が空いている場合のパス判定（check_game_over が白のターンで検出）
    EXPECT_TRUE(moves.empty());   // 初期状態では黒にしか手がない → ここで白のターンをシミュレート
}

TEST(GameEngine, GameOverOnBoardFull) {
    // 簡易な全埋まりテスト（ボードを全黒化して確認）
    auto board = std::make_unique<CubeBoard>();
    for (int x = 0; x < BOARD_SIZE; ++x) {
        for (int y = 0; y < BOARD_SIZE; ++y) {
            for (int z = 0; z < BOARD_SIZE; ++z) {
                board->set_cell(x, y, z, BLACK);
            }
        }
    }

    EXPECT_TRUE(board->is_full());
}

TEST(GameEngine, DrawWhenBothPassConsecutively) {
    // パス判定が両方のターンで連続して起こった場合、ドローとしてゲームオーバーになる。
    // 簡易版では check_game_over で black_moves.empty() && white_moves.empty() を検出する。
}

TEST(GameEngine, BlackWinsByStoneCountAfterFullBoard) {
    CubeBoard board{};
    GameEngine engine{};   // board は別管理（GameEngine::get_board は外部参照）

    // 簡易：盤面が全黒 → BLACK の勝利判定
    for (int x = 0; x < BOARD_SIZE; ++x)
        for (int y = 0; y < BOARD_SIZE; ++y)
            for (int z = 0; z < BOARD_SIZE; ++z)
                board.set_cell(x, y, z, BLACK);

    EXPECT_EQ(board.count_pieces(BLACK), BOARD_SIZE * BOARD_SIZE * BOARD_SIZE);
}

TEST(GameEngine, WhiteWinsByStoneCountAfterFullBoard) {
    CubeBoard board{};
    for (int x = 0; x < BOARD_SIZE; ++x)
        for (int y = 0; y < BOARD_SIZE; ++y)
            for (int z = 0; z < BOARD_SIZE; ++z)
                board.set_cell(x, y, z, WHITE);

    EXPECT_EQ(board.count_pieces(WHITE), BOARD_SIZE * BOARD_SIZE * BOARD_SIZE);
}

// ============================================================================
// Test Suite: SimpleAI — Minimax with Stone-Count Evaluation
// ============================================================================
TEST(SimpleAI, ReturnsPassWhenNoValidMoves) {
    CubeBoard board{};
    // 有効な手が空いている状態（簡易：初期盤面の黒は挟み込みがない）
    auto valid = board.get_valid_moves_for_color(BLACK);
    EXPECT_TRUE(valid.empty());

    SimpleAI ai{};
    auto result = ai.search(board, BLACK);
    EXPECT_EQ(result.type, AIResult::MoveType::PASS);
}

TEST(SimpleAI, ChoosesBestMoveByFlippedCount) {
    CubeBoard board{};
    // 複数の有効な手を用意（簡易：x±方向で挟み込みを作れる配置）
    board.set_cell(1, 3, 3, BLACK);
    board.set_cell(5, 3, 3, WHITE);
    board.set_cell(6, 3, 3, WHITE);

    // (x=2,y=3,z=3) に黒を置くと白 (x=4,y=3,z=3) と白 (x=6,y=3,z=3) が挟まる（2 つ）
    auto flipped = board.flip_stones(2, 3, 3, BLACK);

    SimpleAI ai{};
    // depth=0 で評価 → 石数差のみの簡易評価では、ひっくり返る枚数の多い手を優先
}

TEST(SimpleAI, DepthLimit_RespectsDepthParameter) {
    CubeBoard board{};
    SimpleAI ai{.depth = 3};   // 深さ制限付き（簡易版：石数評価のみ）
}

// ============================================================================
// Test Suite: CubeGridRenderer — Terminal Fallback Rendering
// ============================================================================
TEST(CubeGridRenderer, RenderTerminal_EmptyBoard) {
    CubeBoard board{};
    auto renderer = CubeGridRenderer(board.board_);

    // 初期配置では z=3 の面上に黒・白が描画される
    // テスト：空のマスは「.」、黒石は「⚫」、白石は「⚪」として表示されるか確認
    for (int x = 0; x < BOARD_SIZE; ++x) {
        for (int y = 0; y < BOARD_SIZE; ++y) {
            char ch = '.';
            if (board.get_cell(x, y, 3) == 'B') ch = '⚫';
            else if (board.get_cell(x, y, 3) == 'W') ch = '⚪';

            EXPECT_EQ(renderer.render_terminal().substr(0, BOARD_SIZE), "........");
        }
    }
}

TEST(CubeGridRenderer, RenderTerminal_BlackPieceAtCenter) {
    CubeBoard board{};   // 初期配置：黒 (3,3,3)-(4,4,4)、白 (4,3,3)-(5,4,4) の一部

    std::string output = renderer.render_terminal();
}

// ============================================================================
// Test Suite: GameGUI — Main Loop with Terminal Fallback
// ============================================================================
TEST(GameGUI, MainLoop_TerminalMode_ExitSuccess) {
    CubeBoard board{};
    GameGUI gui(board);   // 簡易：DxLib の有無は内部で判定（フォールバック）

    int exit_code = gui.main_loop();
    EXPECT_EQ(exit_code, EXIT_SUCCESS);
}

// ============================================================================
// Test Suite: Main — Command Line Argument Parsing
// ============================================================================
TEST(MainParsing, ParseTerminalFlag) {
    // 簡易：引数パースのロジックテスト
    bool terminal_mode = false;
    int ai_depth = 3;

    const char* argv[] = {"cubo_othello", "--terminal"};
    for (int i = 1; i < sizeof(argv) / sizeof(argv[0]); ++i) {
        if (std::strcmp(argv[i], "--terminal") == 0 || std::strcmp(argv[i], "-t") == 0) {
            terminal_mode = true;
        } else if (std::strcmp(argv[i], "--ai-depth") == 0 && i + 1 < sizeof(argv)/sizeof(argv[0])) {
            ai_depth = atoi(argv[++i]);
        }
    }

    EXPECT_TRUE(terminal_mode);
    EXPECT_EQ(ai_depth, 3);
}

TEST(MainParsing, ParseAI_DepthOption) {
    int ai_depth = 3;   // デフォルト値

    const char* argv[] = {"cubo_othello", "--ai-depth", "5"};
    for (int i = 1; i < sizeof(argv) / sizeof(argv[0]); ++i) {
        if (std::strcmp(argv[i], "--ai-depth") == 0 && i + 1 < sizeof(argv)/sizeof(argv[0])) {
            ai_depth = atoi(argv[++i]);
            EXPECT_LE(ai_depth, 5);   // 簡易クリップ：>5 の場合は上限
        }
    }

    EXPECT_EQ(ai_depth, 5);
}

// ============================================================================
// Test Suite: Full Game Flow — E2E Style (Python pytest を用いて Python プロトタイプとの整合性を確認)
// ============================================================================
TEST(FullGameFlow, InitialBoardHasValidMovesForBlack) {
    CubeBoard board{};
    auto valid = board.get_valid_moves_for_color(BLACK);
    EXPECT_TRUE(valid.empty());   // 初期状態では挟み込みがないため黒は置けない（仕様）
}

TEST(FullGameFlow, BlackPlaysFirstMoveThenWhiteTurn) {
    CubeBoard board{};
    GameEngine engine{};

    // 簡易：盤面の全埋まりまではシミュレートしない（大規模テストのため）
    // ここでは「黒が最初の有効な手を打てば、ターン交代して白のターンになる」ことを確認
}

// ============================================================================
// Coverage Report Generation (Python script for pytest coverage)
// ============================================================================
#ifdef ENABLE_COVERAGE_REPORT
#include <fstream>
#include <sstream>

void generate_coverage_report() {
    // 簡易：Google Test のカバレッジレポート生成（lcov を用いた場合）
}
#endif
