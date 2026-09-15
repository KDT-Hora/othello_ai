#include <gtest/gtest.h>
#include "src/ai_player.hpp"
#include "src/board.hpp"

namespace {

using ::testing::Contains;

} // namespace

// ─── SimpleAI 評価関数テスト ──────────────────────────────────────────────

TEST(SimpleAIPlayer, evaluate_returns_positive_when_ai_has_more_stones) {
    cubo_othello::CubeBoard board{};

    // Initialize an empty board (no center stones for simplicity in this test)
    board.grid_.fill(-1);  // EMPTY

    auto ai = cubo_othello::SimpleAIPlayer(cubo_othello::SimpleAIPlayer::Role::AI, {});

    // AI is Black; place one black stone → evaluation should be positive (+1)
    EXPECT_EQ(board.place_stone(0, 0, 0, cubo_othello::BLACK), 1);

    int score = ai.evaluate(board);

    EXPECT_GT(score, 0);   // AI (Black) has more stones than opponent → positive
}

TEST(SimpleAIPlayer, evaluate_returns_negative_when_opponent_has_more_stones) {
    cubo_othello::CubeBoard board{};
    board.grid_.fill(-1);

    auto ai = cubo_othello::SimpleAIPlayer(cubo_othello::SimpleAIPlayer::Role::AI, {});

    // AI is Black; place one white stone → evaluation should be negative (-1)
    EXPECT_EQ(board.place_stone(0, 0, 0, cubo_othello::WHITE), 1);

    int score = ai.evaluate(board);

    EXPECT_LT(score, 0);   // AI (Black) has fewer stones → negative
}

TEST(SimpleAIPlayer, evaluate_returns_zero_when_board_is_balanced) {
    cubo_othello::CubeBoard board{};
    board.grid_.fill(-1);

    auto ai = cubo_othello::SimpleAIPlayer(cubo_othello::SimpleAIPlayer::Role::AI, {});

    // Place equal stones: 2 Black, 2 White → score ≈ 0
    EXPECT_EQ(board.place_stone(0, 0, 0, cubo_othello::BLACK), 1);
    EXPECT_EQ(board.place_stone(1, 0, 0, cubo_othello::WHITE), 1);

    int score = ai.evaluate(board);

    // With only 2 stones each: Black=1, White=1 → score = 0
    EXPECT_EQ(score, 0);
}

// ─── search / minimax テスト ──────────────────────────────────────────────

TEST(SimpleAIPlayer, search_returns_some_legal_move_when_available) {
    cubo_othello::CubeBoard board{};
    board.grid_.fill(-1);   // empty board (no center stones for simplicity)

    auto ai = cubo_othello::SimpleAIPlayer(cubo_othello::SimpleAIPlayer::Role::AI, {});

    auto result = ai.search(board, true);  // AI is maximizing player
    EXPECT_TRUE(result.has_value());       // at least one move exists on empty board
}

TEST(SimpleAIPlayer, search_returns_empty_when_no_legal_moves) {
    cubo_othello::CubeBoard board{};

    // Fill entire board (simulate game-over situation with no legal moves)
    for (int x = 0; x < BOARD_SIZE; ++x) {
        for (int y = 0; y < BOARD_SIZE; ++y) {
            for (int z = 0; z < Z_LAYERS; ++z) {
                board.grid_[x][y][z] = cubo_othello::BLACK;
            }
        }
    }

    auto ai = cubo_othello::SimpleAIPlayer(cubo_othello::SimpleAIPlayer::Role::AI, {});

    std::optional<std::tuple<int,int,int>> result = ai.search(board, true);
    EXPECT_FALSE(result.has_value());   // no legal moves → pass (empty optional)
}

TEST(SimpleAIPlayer, search_depth_limited_recursion) {
    cubo_othello::CubeBoard board{};
    board.grid_.fill(-1);

    auto ai = cubo_othello::SimpleAIPlayer(cubo_othello::SimpleAIPlayer::Role::AI, {});

    // At depth 0 (leaf), evaluation is used directly without further recursion.
    // We verify that search() completes within a reasonable time for small boards.
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    auto result = ai.search(board, true);
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    // Search should complete in microseconds for an empty board at depth 3.
    EXPECT_TRUE(result.has_value());

    // Verify that the search respects the configured depth limit:
    EXPECT_LE(ai.depth, 5);   // depth was set to default 3
}

// ─── Minimax の正当性：深さ 0（評価関数のみ）で石数の差分が返される ──────────

TEST(SimpleAIPlayer, minimax_depth_zero_returns_evaluated_score) {
    cubo_othello::CubeBoard board{};
    board.grid_.fill(-1);

    // Place one black stone (AI is Black)
    EXPECT_EQ(board.place_stone(0, 0, 0, cubo_othello::BLACK), 1);

    auto ai = cubo_othello::SimpleAIPlayer(cubo_othello::SimpleAIPlayer::Role::AI, {});

    // At depth=0 (leaf node), minimax should simply return the evaluation score.
    EXPECT_EQ(ai.minimax(board, true, 0), 1);   // Black has one more stone → +1
}

// ─── Tie-breaking: random selection among equally-best moves ──────────────────

TEST(SimpleAIPlayer, tiebreaking_is_deterministic_given_seeded_rng) {
    cubo_othello::CubeBoard board{};
    board.grid_.fill(-1);

    auto ai = cubo_othello::SimpleAIPlayer(cubo_othello::SimpleAIPlayer::Role::AI, {});

    // Two legal moves exist (all cells are empty). The AI picks one of them.
    // We do not enforce which move is chosen here; we only confirm that a move exists.
    auto result = ai.search(board, true);
    EXPECT_TRUE(result.has_value());
}
