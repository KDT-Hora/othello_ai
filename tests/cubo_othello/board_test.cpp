#include <gtest/gtest.h>
#include "src/board.hpp"

namespace {

using ::testing::Contains;

} // namespace

// ─── CuboBoard 初期化テスト ──────────────────────────────────────────────────────

TEST(CubeBoard, initialization) {
    cubo_othello::CubeBoard board{};
    board.initialize();

    // ✓ Center stones are placed at (x,y,z) ∈ {3,4}×{3,4}×{3,5}
    EXPECT_EQ(board.grid[3][3][3], 0);   // Black
    EXPECT_EQ(board.grid[3][4][3], 0);   // Black
    EXPECT_EQ(board.grid[4][3][3], 1);   // White
    EXPECT_EQ(board.grid[4][4][3], 1);   // White

    EXPECT_EQ(board.grid[3][3][4], 0);   // Black
    EXPECT_EQ(board.grid[3][4][4], 0);   // Black
    EXPECT_EQ(board.grid[4][3][4], 1);   // White
    EXPECT_EQ(board.grid[4][4][4], 1);   // White

    // All other cells are empty (-1)
    for (int x = 0; x < BOARD_SIZE; ++x) {
        for (int y = 0; y < BOARD_SIZE; ++y) {
            for (int z = 0; z < Z_LAYERS; ++z) {
                if ((x == 3 || x == 4) && (y == 3 || y == 4) &&
                    (z == 3 || z == 5)) {
                    // center region → not empty
                    EXPECT_NE(board.grid[x][y][z], -1);
                } else {
                    EXPECT_EQ(board.grid[x][y][z], static_cast<int>(EMPTY));
                }
            }
        }
    }

    EXPECT_EQ(board.turn, BLACK);
    EXPECT_FALSE(board.game_over());
}

// ─── 石の配置テスト ──────────────────────────────────────────────────────────────

TEST(CubeBoard, place_stone_basic) {
    cubo_othello::CubeBoard board{};
    board.initialize();

    // Place black stone at (0,0,0) — should be valid and flip nothing (corner cell is empty)
    int flipped = board.place_stone(0, 0, 0, BLACK);
    EXPECT_EQ(flipped, 1);   // placed one stone itself; no sandwich → zero flips recorded
}

TEST(CubeBoard, place_stone_invalid_location) {
    cubo_othello::CubeBoard board{};
    board.initialize();

    // Cannot place on an occupied cell — must return 0 (invalid move)
    EXPECT_EQ(board.place_stone(3, 3, 3, WHITE), 0);   // already Black
}

TEST(CubeBoard, get_valid_moves_after_init) {
    cubo_othello::CubeBoard board{};
    board.initialize();

    auto moves = board.get_valid_moves(BLACK);
    EXPECT_GT(moves.size(), static_cast<size_t>(0));   // at least one valid move exists after init
}

// ─── ゲームオーバー判定テスト ──────────────────────────────────────────────────

TEST(CubeBoard, game_over_no_moves) {
    cubo_othello::CubeBoard board{};
    board.initialize();

    // Simulate a situation where no player has valid moves (full board or deadlocked)
    EXPECT_FALSE(board.game_over());   // freshly initialized → still in progress

    // Fill the rest of the board with stones to trigger full-board draw
    for (int x = 0; x < BOARD_SIZE; ++x) {
        for (int y = 0; y < BOARD_SIZE; ++y) {
            for (int z = 0; z < Z_LAYERS; ++z) {
                if ((x,y,z) == {3,3,3} || (x,y,z) == {4,4,4}) continue;   // already filled
                board.grid[x][y][z] = BLACK;
            }
        }
    }

    EXPECT_TRUE(board.game_over());   // full board → draw condition
}

// ─── 石の個数カウントテスト ──────────────────────────────────────────────────────

TEST(CubeBoard, count_pieces) {
    cubo_othello::CubeBoard board{};
    board.initialize();

    EXPECT_EQ(board.count_pieces(BLACK), 4);   // center Black stones: (3,3,z)×{z∈{3..5}} → 4 pieces
    EXPECT_EQ(board.count_pieces(WHITE), 4);   // same for White
}
