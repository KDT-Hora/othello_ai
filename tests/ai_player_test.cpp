#include <gtest/gtest.h>

#include <algorithm>

#include "ai_player.hpp"
#include "board.hpp"

using cubo_othello::BLACK;
using cubo_othello::CubeBoard;
using cubo_othello::Move;
using cubo_othello::SimpleAI;
using cubo_othello::WHITE;

namespace {
bool is_among(const Move& move, const std::vector<Move>& candidates) {
    return std::any_of(candidates.begin(), candidates.end(), [&](const Move& c) {
        return c.x == move.x && c.y == move.y && c.z == move.z;
    });
}
} // namespace

TEST(SimpleAI, EvaluateIsFromOwnColorPerspective) {
    CubeBoard board;
    board.clear();
    board.set_for_testing(0, 0, 0, BLACK);
    board.set_for_testing(1, 0, 0, WHITE);
    board.set_for_testing(2, 0, 0, WHITE);

    SimpleAI black_ai(BLACK);
    SimpleAI white_ai(WHITE);

    EXPECT_EQ(black_ai.evaluate(board), 1 - 2);
    EXPECT_EQ(white_ai.evaluate(board), 2 - 1);
}

TEST(SimpleAI, ChooseMoveReturnsNulloptWhenNoLegalMoves) {
    CubeBoard board;
    board.clear();
    board.set_for_testing(0, 0, 0, BLACK); // isolated stone: nobody can move

    SimpleAI ai(BLACK, 3);
    EXPECT_FALSE(ai.choose_move(board).has_value());
}

TEST(SimpleAI, ChooseMoveNeverReturnsAnIllegalMove) {
    CubeBoard board; // standard opening
    SimpleAI ai(BLACK, 3);

    auto move = ai.choose_move(board);
    ASSERT_TRUE(move.has_value());

    auto legal = board.valid_moves(BLACK);
    EXPECT_TRUE(is_among(*move, legal));
}

TEST(SimpleAI, ChooseMoveDoesNotMutateTheBoard) {
    CubeBoard board;
    SimpleAI ai(WHITE, 2);
    ai.choose_move(board);
    // WHITE never had a turn on the fresh opening board; it must be untouched.
    EXPECT_EQ(board.count(BLACK), 4);
    EXPECT_EQ(board.count(WHITE), 4);
}

TEST(SimpleAI, ChooseMovePrefersLargerImmediateFlipAtDepthOne) {
    CubeBoard board;
    board.clear();

    // Option A: placing BLACK at (0,0,0) flips a single White stone.
    board.set_for_testing(1, 0, 0, WHITE);
    board.set_for_testing(2, 0, 0, BLACK);

    // Option B: placing BLACK at (0,5,5) flips a run of three White stones.
    board.set_for_testing(1, 5, 5, WHITE);
    board.set_for_testing(2, 5, 5, WHITE);
    board.set_for_testing(3, 5, 5, WHITE);
    board.set_for_testing(4, 5, 5, BLACK);

    SimpleAI ai(BLACK, /*depth=*/1);
    auto move = ai.choose_move(board);
    ASSERT_TRUE(move.has_value());

    EXPECT_EQ(move->x, 0);
    EXPECT_EQ(move->y, 5);
    EXPECT_EQ(move->z, 5);
}

TEST(SimpleAI, ChooseMovePassesRandomnessButAlwaysLegalAcrossManyTrials) {
    CubeBoard board;
    for (int trial = 0; trial < 20; ++trial) {
        SimpleAI ai(BLACK, 2);
        auto move = ai.choose_move(board);
        ASSERT_TRUE(move.has_value());
        EXPECT_TRUE(is_among(*move, board.valid_moves(BLACK)));
    }
}
