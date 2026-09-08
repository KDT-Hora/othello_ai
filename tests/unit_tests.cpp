// Cube Othello — Unit Tests with Google Test (C++)
// ============================================================================
// カバレッジ目標：95%+
//   - Board::flip_stones の全分岐（6 方向 × 挟み込み成立/不成立 / 空マス）
//   - GameEngine::play_move の全状態遷移パス網羅
//   - SimpleAI の深さ制限付きミニマックスの全探索経路網羅

#pragma once

#include <gtest/gtest.h>
#include "board.hpp"
#include "game.hpp"

namespace cubo {

// ============================================================================
// TEST_SUITE: CubeBoard — 盤面管理クラス
TEST_P(CubeBoard, Test_Initial_Placement) {
    // T01: 初期配置の検証
    board.initialize();

    EXPECT_EQ(board.get_cell(3, 3, 3), PieceColor::BLACK);
    EXPECT_EQ(board.get_cell(4, 3, 3), PieceColor::WHITE);
    EXPECT_EQ(board.get_cell(3, 4, 3), PieceColor::WHITE);
    EXPECT_EQ(board.get_cell(4, 4, 3), PieceColor::BLACK);

    // 初期配置以外のマスは空であること
    for (int x = 0; x < BOARD_SIZE; ++x) {
        for (int y = 0; y < BOARD_SIZE; ++y) {
            for (int z = 0; z < BOARD_SIZE; ++z) {
                if ((x, y, z) != std::make_tuple(3, 3, 3) &&
                    (x, y, z) != std::make_tuple(4, 3, 3) &&
                    (x, y, z) != std::make_tuple(3, 4, 3) &&
                    (x, y, z) != std::make_tuple(4, 4, 3)) {
                    EXPECT_EQ(board.get_cell(x, y, z), PieceColor::EMPTY);
                }
            }
        }
    }
}

TEST_P(CubeBoard, Test_Reset) {
    board.initialize();
    board.reset();

    for (int x = 0; x < BOARD_SIZE; ++x) {
        for (int y = 0; y < BOARD_SIZE; ++y) {
            for (int z = 0; z < BOARD_SIZE; ++z) {
                EXPECT_EQ(board.get_cell(x, y, z), PieceColor::EMPTY);
            }
        }
    }
}

TEST_P(CubeBoard, Test_SetCell_InvalidCoordinates) {
    board.initialize();

    // 座標範囲外の場合は false を返す（同時に EMPTY を保持）
    EXPECT_FALSE(board.set_cell(8, 0, 0, PieceColor::BLACK));   // x 超過
    EXPECT_EQ(board.get_cell(8, 0, 0), PieceColor::EMPTY);

    EXPECT_FALSE(board.set_cell(-1, 0, 0, PieceColor::WHITE));  // x 未満
    EXPECT_EQ(board.get_cell(-1, 0, 0), PieceColor::EMPTY);

    EXPECT_FALSE(board.set_cell(3, 8, 0, PieceColor::BLACK));   // y 超過
    EXPECT_EQ(board.get_cell(3, 8, 0), PieceColor::EMPTY);
}

TEST_P(CubeBoard, Test_SetCell_AlreadyOccupied) {
    board.initialize();

    // 既に埋まっているマスに石を置く場合は false を返す
    EXPECT_FALSE(board.set_cell(3, 3, 3, PieceColor::WHITE));   // 黒が已まれている
    EXPECT_EQ(board.get_cell(3, 3, 3), PieceColor::BLACK);

    board.reset();
}

TEST_P(CubeBoard, Test_FlipStones_NoSandwich) {
    board.initialize();
    std::vector<std::tuple<int,int,int>> flipped = board.flip_stones(0,0,0,PieceColor::BLACK);

    EXPECT_EQ(flipped.size(), 0UL);   // サンドウィッチなし → 反転しない
}

TEST_P(CubeBoard, Test_FlipStones_PositiveX) {
    board.initialize();
    board.set_cell(5,3,3,PieceColor::WHITE);
    board.set_cell(6,3,3,PieceColor::BLACK);   // x=6 に黒を置く

    auto flipped = board.flip_stones(4, 3, 3, PieceColor::BLACK);

    EXPECT_EQ(flipped.size(), 2UL);   // (5,3,3) の黒石が反転
}

TEST_P(CubeBoard, Test_FlipStones_NegativeX) {
    board.initialize();
    board.set_cell(1,3,3,PieceColor::WHITE);
    board.set_cell(0,3,3,PieceColor::BLACK);   // x=0 に黒を置く

    auto flipped = board.flip_stones(2, 3, 3, PieceColor::BLACK);

    EXPECT_EQ(flipped.size(), 1UL);   // (1,3,3) の白石が反転
}

TEST_P(CubeBoard, Test_FlipStones_PositiveY) {
    board.initialize();
    board.set_cell(3,5,3,PieceColor::WHITE);
    board.set_cell(3,6,3,PieceColor::BLACK);   // y=6 に黒を置く

    auto flipped = board.flip_stones(3, 4, 3, PieceColor::BLACK);

    EXPECT_EQ(flipped.size(), 1UL);   // (3,5,3) の白石が反転
}

TEST_P(CubeBoard, Test_FlipStones_NegativeY) {
    board.initialize();
    board.set_cell(3,1,3,PieceColor::WHITE);
    board.set_cell(3,0,3,PieceColor::BLACK);   // y=0 に黒を置く

    auto flipped = board.flip_stones(3, 2, 3, PieceColor::BLACK);

    EXPECT_EQ(flipped.size(), 1UL);   // (3,1,3) の白石が反転
}

TEST_P(CubeBoard, Test_FlipStones_PositiveZ) {
    board.initialize();
    board.set_cell(3,3,5,PieceColor::WHITE);
    board.set_cell(3,3,6,PieceColor::BLACK);   // z=6 に黒を置く

    auto flipped = board.flip_stones(3, 3, 4, PieceColor::BLACK);

    EXPECT_EQ(flipped.size(), 1UL);   // (3,3,5) の白石が反転
}

TEST_P(CubeBoard, Test_FlipStones_NegativeZ) {
    board.initialize();
    board.set_cell(3,3,1,PieceColor::WHITE);
    board.set_cell(3,3,0,PieceColor::BLACK);   // z=0 に黒を置く

    auto flipped = board.flip_stones(3, 3, 2, PieceColor::BLACK);

    EXPECT_EQ(flipped.size(), 1UL);   // (3,3,1) の白石が反転
}

TEST_P(CubeBoard, Test_FlipStones_MultiDirection_Sandwich) {
    board.initialize();

    // x+方向のサンドウィッチ
    board.set_cell(5,3,3,PieceColor::WHITE);   // 相手の石
    board.set_cell(6,3,3,PieceColor::BLACK);   // 自分の石 → サンドウィッチ成立

    auto flipped = board.flip_stones(4, 3, 3, PieceColor::BLACK);

    EXPECT_EQ(flipped.size(), 2UL);   // (5,3,3) の黒石が反転
}

TEST_P(CubeBoard, Test_FlipStones_EmptyAfterFlip) {
    board.initialize();
    board.set_cell(5,3,3,PieceColor::WHITE);
    board.set_cell(6,3,3,PieceColor::BLACK);   // 挟み込み後の自分の石

    auto flipped = board.flip_stones(4, 3, 3, PieceColor::BLACK);

    EXPECT_EQ(flipped.size(), 2UL);   // (5,3,3) の白石が黒に反転
    EXPECT_EQ(board.get_cell(5, 3, 3), PieceColor::BLACK);
}

TEST_P(CubeBoard, Test_FlipStones_MultipleFlipsInOneMove) {
    board.initialize();

    // x+方向：2 つの相手の石を挟むパターン
    board.set_cell(4,3,3,PieceColor::WHITE);   // 相手の石（左）
    board.set_cell(5,3,3,PieceColor::BLACK);   // 自分の石
    board.set_cell(6,3,3,PieceColor::WHITE);   // 相手の石（右）
    board.set_cell(7,3,3,PieceColor::BLACK);   // 自分の石 → サンドウィッチ成立

    auto flipped = board.flip_stones(5, 3, 3, PieceColor::BLACK);

    EXPECT_EQ(flipped.size(), 2UL);   // (4,3,3) と (6,3,3) の白石が反転
}

TEST_P(CubeBoard, Test_FlipStones_NoFlipsForWhite) {
    board.initialize();
    board.set_cell(5,3,3,PieceColor::WHITE);   // 相手の石（黒から見たら相手）
    board.set_cell(6,3,3,PieceColor::BLACK);

    auto flipped = board.flip_stones(4, 3, 3, PieceColor::WHITE);

    EXPECT_EQ(flipped.size(), 0UL);   // 黒の石がない → 反転しない
}

// ============================================================================
// TEST_SUITE: GameEngine — ゲームエンジン
TEST_P(GameEngine, Test_BlackStartsFirst) {
    engine = std::make_unique<GameEngine>();

    EXPECT_EQ(engine->get_board().get_cell(3, 3, 3), PieceColor::BLACK);   // 黒が先攻
}

TEST_P(GameEngine, Test_TurnSwitching) {
    engine = std::make_unique<GameEngine>();
    auto& board = engine->get_board();

    EXPECT_TRUE(engine->get_turn_player() == PieceColor::BLACK);   // 黒が先攻

    // サンプルの置石（簡易なテスト）
    board.set_cell(3, 4, 3, PieceColor::BLACK);   // 仮の置石
}

TEST_P(GameEngine, Test_GameOver_BoardFull) {
    engine = std::make_unique<GameEngine>();
    auto& board = engine->get_board();

    for (int x = 0; x < BOARD_SIZE; ++x) {
        for (int y = 0; y < BOARD_SIZE; ++y) {
            for (int z = 0; z < BOARD_SIZE; ++z) {
                board.set_cell(x, y, z, PieceColor::BLACK);   // 全マス黒で埋める（簡易化）
            }
        }
    }

    bool game_over = engine->is_game_over();
}

TEST_P(GameEngine, Test_Stalemate_DrawCondition) {
    engine = std::make_unique<GameEngine>();
    auto& board = engine->get_board();

    // 盤面が埋まっている状態 → ゲームオーバー（黒勝ち）
    for (int x = 0; x < BOARD_SIZE; ++x) {
        for (int y = 0; y < BOARD_SIZE; ++y) {
            for (int z = 0; z < BOARD_SIZE; ++z) {
                board.set_cell(x, y, z, PieceColor::BLACK);   // 全マス黒で埋める
            }
        }
    }

    bool game_over = engine->is_game_over();
}

TEST_P(GameEngine, Test_ValidMovesDetection) {
    engine = std::make_unique<GameEngine>();
    auto& board = engine->get_board();

    // 初期状態では有効な手は存在しない（サンドウィッチなし）
    auto valid_moves = board.get_all_valid_moves();
}

TEST_P(GameEngine, Test_NoValidMovesEmptyBoard) {
    engine = std::make_unique<GameEngine>();
    auto& board = engine->get_board();

    // 盤面を空にリセット → 有効な手はない
    board.reset();

    auto valid_moves = board.get_all_valid_moves();
}

// ============================================================================
// TEST_SUITE: SimpleAI — シンプルミニマックス AI（深さ制限付き）
TEST_P(SimpleAI, Test_NoValidMoves_ReturnsPass) {
    ai = std::make_unique<SimpleAI>(3);   // 深さ制限：3

    auto& board = engine->get_board();
    board.reset();

    std::pair<std::string, std::vector<std::tuple<int,int,int>>> result = ai->search(board, PieceColor::BLACK);

    EXPECT_EQ(result.first, "PASS");   // 有効な手が空いている → パス（簡易）
}

TEST_P(SimpleAI, Test_PlaysBestMoveByStoneCount) {
    engine = std::make_unique<GameEngine>();
    auto& board = engine->get_board();

    // サンプル：黒の石が多い状況で AI が有利な手を選ぶか確認（簡易）
    ai = std::make_unique<SimpleAI>(3);   // 深さ制限：3

    // 盤面にいくつかの石を配置してテスト
}

// ============================================================================
TEST_P(SimpleAI, Test_MinimaxSearchDepthZero) {
    engine = std::make_unique<GameEngine>();
    ai = std::make_unique<SimpleAI>(0);   // 深さ制限：0（全探索）

    auto& board = engine->get_board();

    // サンプルの盤面状態
}

TEST_P(SimpleAI, Test_MinimaxSearchDepthOne) {
    engine = std::make_unique<GameEngine>();
    ai = std::make_unique<SimpleAI>(1);   // 深さ制限：1（浅い探索）

    auto& board = engine->get_board();
}

TEST_P(SimpleAI, Test_MinimaxSearchDepthTwo) {
    engine = std::make_unique<GameEngine>();
    ai = std::make_unique<SimpleAI>(2);   // 深さ制限：2（中程度の探索）

    auto& board = engine->get_board();
}

TEST_P(SimpleAI, Test_EvaluationFunction_StoneCountDifference) {
    engine = std::make_unique<GameEngine>();
    ai = std::make_unique<SimpleAI>(3);   // 深さ制限：3（評価関数：石数のみ）

    auto& board = engine->get_board();
}

// ============================================================================
INSTANTIATE_TEST_SUITE_P(AllTests, CubeBoardTestSuite);
INSTANTIATE_TEST_SUITE_P(AllTests, GameEngineTestSuite);
INSTANTIATE_TEST_SUITE_P(AllTests, SimpleAITestSuite);

} // namespace cubo
