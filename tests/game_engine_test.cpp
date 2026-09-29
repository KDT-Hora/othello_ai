// Exercises GameEngine's turn/pass/undo bookkeeping without opening a real
// DXLib window: GameEngineTestAccess reaches the private members that
// run()'s DXLib loop normally drives.
#include <gtest/gtest.h>

#include "game_engine.hpp"

namespace cubo_othello {

struct GameEngineTestAccess {
    static CubeBoard& board(GameEngine& e) { return e.board_; }
    static int8_t& turn(GameEngine& e) { return e.turn_; }
    static size_t history_size(const GameEngine& e) { return e.history_.size(); }
    static void advance_turn(GameEngine& e) { e.advance_turn(); }
    static void push_history(GameEngine& e) { e.push_history(); }
    static void undo(GameEngine& e) { e.undo(); }
};

namespace {
using Access = GameEngineTestAccess;
} // namespace

TEST(GameEngine, AdvanceTurnSwitchesToOpponentWhenTheyHaveAMove) {
    GameEngine engine; // human vs human
    Access::turn(engine) = BLACK;
    Access::advance_turn(engine);
    EXPECT_EQ(Access::turn(engine), WHITE); // opening position: White always has moves
}

TEST(GameEngine, AdvanceTurnPassesBackWhenOpponentHasNoMove) {
    GameEngine engine;
    CubeBoard& board = Access::board(engine);
    board.clear();
    // A single stone and no opponent stones anywhere: WHITE has no legal move.
    board.set_for_testing(0, 0, 0, BLACK);
    Access::turn(engine) = BLACK;

    Access::advance_turn(engine);

    EXPECT_EQ(Access::turn(engine), BLACK); // WHITE passes, BLACK keeps the turn
}

TEST(GameEngine, UndoWithEmptyHistoryIsANoOp) {
    GameEngine engine;
    CubeBoard before = Access::board(engine);
    int8_t turn_before = Access::turn(engine);

    Access::undo(engine);

    EXPECT_EQ(Access::turn(engine), turn_before);
    EXPECT_EQ(Access::board(engine).evaluate(), before.evaluate());
    EXPECT_EQ(Access::history_size(engine), 0u);
}

TEST(GameEngine, UndoInHumanVsHumanRestoresThePreviousSnapshot) {
    GameEngine engine; // no AI: single-step undo
    CubeBoard& board = Access::board(engine);
    const int black_before = board.count(BLACK);
    const int white_before = board.count(WHITE);

    Access::push_history(engine);
    ASSERT_EQ(board.place_stone(4, 2, 2, BLACK), 1); // a real opening move
    Access::turn(engine) = WHITE;

    ASSERT_NE(board.count(BLACK), black_before); // sanity: the move actually happened

    Access::undo(engine);

    EXPECT_EQ(Access::board(engine).count(BLACK), black_before);
    EXPECT_EQ(Access::board(engine).count(WHITE), white_before);
    EXPECT_EQ(Access::turn(engine), BLACK);
    EXPECT_EQ(Access::history_size(engine), 0u);
}

TEST(GameEngine, UndoWithAiSkipsBackPastTheAisReplyToTheHumansTurn) {
    // AI plays White; simulate: Black (human) moves, then White (AI) moves.
    GameEngine engine(WHITE, /*ai_depth=*/1);
    CubeBoard& board = Access::board(engine);

    Access::push_history(engine); // snapshot before Black's move
    ASSERT_EQ(board.place_stone(4, 2, 2, BLACK), 1);
    Access::turn(engine) = WHITE;

    Access::push_history(engine); // snapshot before White(AI)'s move
    // (4,3,2) sandwiches the untouched (3,3,2) Black stone against the
    // original (2,3,2) White anchor -- independent of Black's move above.
    ASSERT_EQ(board.place_stone(4, 3, 2, WHITE), 1);
    Access::turn(engine) = BLACK;

    ASSERT_EQ(Access::history_size(engine), 2u);

    Access::undo(engine);

    // Both the AI's reply and the human's move that triggered it are undone,
    // landing back on Black's original turn with the original stone counts.
    EXPECT_EQ(Access::turn(engine), BLACK);
    EXPECT_EQ(Access::board(engine).count(BLACK), 4);
    EXPECT_EQ(Access::board(engine).count(WHITE), 4);
    EXPECT_EQ(Access::history_size(engine), 0u);
}

TEST(GameEngine, UndoAtTheVeryFirstAiMoveReturnsToTheInitialPositionAndAisTurn) {
    // AI plays Black (moves first). Undoing its only move can't hand control
    // to a human turn that never existed -- it lands back on the initial
    // position with Black (the AI) still to move.
    GameEngine engine(BLACK, /*ai_depth=*/1);
    CubeBoard& board = Access::board(engine);

    Access::push_history(engine);
    ASSERT_EQ(board.place_stone(4, 2, 2, BLACK), 1);
    Access::turn(engine) = WHITE;

    Access::undo(engine);

    EXPECT_EQ(Access::turn(engine), BLACK);
    EXPECT_EQ(Access::board(engine).count(BLACK), 4);
    EXPECT_EQ(Access::board(engine).count(WHITE), 4);
    EXPECT_EQ(Access::history_size(engine), 0u);
}

TEST(GameEngine, StartGameResetsBoardTurnAndHistory) {
    GameEngine engine;
    CubeBoard& board = Access::board(engine);
    Access::push_history(engine);
    ASSERT_EQ(board.place_stone(4, 2, 2, BLACK), 1);
    Access::turn(engine) = WHITE;

    engine.start_game(std::nullopt, 1);

    EXPECT_EQ(Access::board(engine).count(BLACK), 4);
    EXPECT_EQ(Access::board(engine).count(WHITE), 4);
    EXPECT_EQ(Access::turn(engine), BLACK);
    EXPECT_EQ(Access::history_size(engine), 0u);
}

TEST(GameEngine, StartGameWithAiColorMakesUndoSkipTheAiReply) {
    GameEngine engine; // human vs human at first
    engine.start_game(WHITE, 1);
    CubeBoard& board = Access::board(engine);

    Access::push_history(engine);
    ASSERT_EQ(board.place_stone(4, 2, 2, BLACK), 1);
    Access::turn(engine) = WHITE;
    Access::push_history(engine);
    ASSERT_EQ(board.place_stone(4, 3, 2, WHITE), 1);
    Access::turn(engine) = BLACK;

    Access::undo(engine); // AI-mode undo unwinds both plies
    EXPECT_EQ(Access::history_size(engine), 0u);
    EXPECT_EQ(Access::turn(engine), BLACK);
}

TEST(GameEngine, StartGameWithoutAiRestoresSingleStepUndo) {
    GameEngine engine(WHITE, 1);
    engine.start_game(std::nullopt, 1);
    CubeBoard& board = Access::board(engine);

    Access::push_history(engine);
    ASSERT_EQ(board.place_stone(4, 2, 2, BLACK), 1);
    Access::turn(engine) = WHITE;
    Access::push_history(engine);
    ASSERT_EQ(board.place_stone(4, 3, 2, WHITE), 1);
    Access::turn(engine) = BLACK;

    Access::undo(engine); // human vs human: one ply only
    EXPECT_EQ(Access::history_size(engine), 1u);
    EXPECT_EQ(Access::turn(engine), WHITE);
}

TEST(GameEngine, SpectateModeUndoIsSingleStep) {
    GameEngine engine;
    engine.start_game(std::nullopt, 1, /*spectate=*/true); // AI vs AI
    CubeBoard& board = Access::board(engine);

    Access::push_history(engine);
    ASSERT_EQ(board.place_stone(4, 2, 2, BLACK), 1);
    Access::turn(engine) = WHITE;
    Access::push_history(engine);
    ASSERT_EQ(board.place_stone(4, 3, 2, WHITE), 1);
    Access::turn(engine) = BLACK;

    Access::undo(engine); // must not skip a second ply just because an AI owns the turn
    EXPECT_EQ(Access::history_size(engine), 1u);
    EXPECT_EQ(Access::turn(engine), WHITE);
}

} // namespace cubo_othello
