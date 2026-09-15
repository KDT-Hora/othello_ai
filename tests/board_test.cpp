#include <gtest/gtest.h>

#include "board.hpp"

using cubo_othello::BLACK;
using cubo_othello::BOARD_SIZE;
using cubo_othello::CubeBoard;
using cubo_othello::EMPTY;
using cubo_othello::GameOverReason;
using cubo_othello::WHITE;

TEST(CubeBoard, InitializePlacesCenterStones) {
    CubeBoard board;

    for (int x : {3, 4}) {
        for (int y : {3, 4}) {
            for (int z : {3, 5}) {
                EXPECT_EQ(board.at(x, y, z), (x == 3) ? BLACK : WHITE)
                    << "at (" << x << "," << y << "," << z << ")";
            }
        }
    }

    int occupied = 0;
    for (int x = 0; x < BOARD_SIZE; ++x)
        for (int y = 0; y < BOARD_SIZE; ++y)
            for (int z = 0; z < BOARD_SIZE; ++z)
                if (board.at(x, y, z) != EMPTY) ++occupied;

    EXPECT_EQ(occupied, 8);
    EXPECT_EQ(board.count(BLACK), 4);
    EXPECT_EQ(board.count(WHITE), 4);
}

TEST(CubeBoard, PlaceStoneOnOccupiedCellIsRejected) {
    CubeBoard board;
    EXPECT_EQ(board.place_stone(3, 3, 3, WHITE), 0);
    EXPECT_EQ(board.at(3, 3, 3), BLACK); // unchanged
}

TEST(CubeBoard, PlaceStoneWithoutSandwichIsRejected) {
    CubeBoard board;
    // (0,0,0) is far from any stone: cannot sandwich anything.
    EXPECT_EQ(board.place_stone(0, 0, 0, BLACK), 0);
    EXPECT_EQ(board.at(0, 0, 0), EMPTY);
}

TEST(CubeBoard, OpeningMoveFlipsOneStone) {
    CubeBoard board; // (3,3,3)=BLACK, (4,3,3)=WHITE by default init
    ASSERT_EQ(board.place_stone(5, 3, 3, BLACK), 1);
    EXPECT_EQ(board.at(5, 3, 3), BLACK);
    EXPECT_EQ(board.at(4, 3, 3), BLACK); // flipped
}

// The following six tests isolate one axial direction each, using
// set_for_testing() to build a minimal, unambiguous sandwich: placed
// stone -> one opponent stone -> one own-color anchor stone.

TEST(CubeBoard, PlaceStoneFlipsAlongPlusX) {
    CubeBoard board;
    board.clear();
    board.set_for_testing(2, 0, 0, WHITE);
    board.set_for_testing(3, 0, 0, BLACK);
    ASSERT_EQ(board.place_stone(1, 0, 0, BLACK), 1);
    EXPECT_EQ(board.at(2, 0, 0), BLACK);
}

TEST(CubeBoard, PlaceStoneFlipsAlongMinusX) {
    CubeBoard board;
    board.clear();
    board.set_for_testing(5, 0, 0, WHITE);
    board.set_for_testing(4, 0, 0, BLACK);
    ASSERT_EQ(board.place_stone(6, 0, 0, BLACK), 1);
    EXPECT_EQ(board.at(5, 0, 0), BLACK);
}

TEST(CubeBoard, PlaceStoneFlipsAlongPlusY) {
    CubeBoard board;
    board.clear();
    board.set_for_testing(0, 2, 0, WHITE);
    board.set_for_testing(0, 3, 0, BLACK);
    ASSERT_EQ(board.place_stone(0, 1, 0, BLACK), 1);
    EXPECT_EQ(board.at(0, 2, 0), BLACK);
}

TEST(CubeBoard, PlaceStoneFlipsAlongMinusY) {
    CubeBoard board;
    board.clear();
    board.set_for_testing(0, 5, 0, WHITE);
    board.set_for_testing(0, 4, 0, BLACK);
    ASSERT_EQ(board.place_stone(0, 6, 0, BLACK), 1);
    EXPECT_EQ(board.at(0, 5, 0), BLACK);
}

TEST(CubeBoard, PlaceStoneFlipsAlongPlusZ) {
    CubeBoard board;
    board.clear();
    board.set_for_testing(0, 0, 2, WHITE);
    board.set_for_testing(0, 0, 3, BLACK);
    ASSERT_EQ(board.place_stone(0, 0, 1, BLACK), 1);
    EXPECT_EQ(board.at(0, 0, 2), BLACK);
}

TEST(CubeBoard, PlaceStoneFlipsAlongMinusZ) {
    CubeBoard board;
    board.clear();
    board.set_for_testing(0, 0, 5, WHITE);
    board.set_for_testing(0, 0, 4, BLACK);
    ASSERT_EQ(board.place_stone(0, 0, 6, BLACK), 1);
    EXPECT_EQ(board.at(0, 0, 5), BLACK);
}

TEST(CubeBoard, PlaceStoneFlipsMultipleDirectionsAtOnce) {
    CubeBoard board;
    board.clear();
    // Two independent sandwiches around a single placement cell (1,1,1).
    board.set_for_testing(2, 1, 1, WHITE);
    board.set_for_testing(3, 1, 1, BLACK);
    board.set_for_testing(1, 2, 1, WHITE);
    board.set_for_testing(1, 3, 1, BLACK);
    ASSERT_EQ(board.place_stone(1, 1, 1, BLACK), 2);
    EXPECT_EQ(board.at(2, 1, 1), BLACK);
    EXPECT_EQ(board.at(1, 2, 1), BLACK);
}

TEST(CubeBoard, RunOfMultipleOpponentStonesAllFlip) {
    CubeBoard board;
    board.clear();
    board.set_for_testing(1, 0, 0, WHITE);
    board.set_for_testing(2, 0, 0, WHITE);
    board.set_for_testing(3, 0, 0, WHITE);
    board.set_for_testing(4, 0, 0, BLACK);
    ASSERT_EQ(board.place_stone(0, 0, 0, BLACK), 3);
    EXPECT_EQ(board.at(1, 0, 0), BLACK);
    EXPECT_EQ(board.at(2, 0, 0), BLACK);
    EXPECT_EQ(board.at(3, 0, 0), BLACK);
}

TEST(CubeBoard, RunEndingInEmptyCellIsNotASandwich) {
    CubeBoard board;
    board.clear();
    board.set_for_testing(1, 0, 0, WHITE);
    // No BLACK anchor beyond the white run -> illegal move.
    EXPECT_EQ(board.place_stone(0, 0, 0, BLACK), 0);
    EXPECT_EQ(board.at(0, 0, 0), EMPTY);
    EXPECT_EQ(board.at(1, 0, 0), WHITE);
}

TEST(CubeBoard, ValidMovesForBlackAtOpeningAreNonEmptyAndOnEmptyCells) {
    CubeBoard board;
    auto moves = board.valid_moves(BLACK);
    EXPECT_FALSE(moves.empty());
    for (const auto& m : moves) {
        EXPECT_EQ(board.at(m.x, m.y, m.z), EMPTY);
    }
}

TEST(CubeBoard, HasValidMovesMatchesValidMoves) {
    CubeBoard board;
    EXPECT_EQ(board.has_valid_moves(BLACK), !board.valid_moves(BLACK).empty());
    EXPECT_EQ(board.has_valid_moves(WHITE), !board.valid_moves(WHITE).empty());
}

TEST(CubeBoard, GameOverReasonInProgressAtOpening) {
    CubeBoard board;
    EXPECT_EQ(board.game_over_reason(), GameOverReason::InProgress);
}

TEST(CubeBoard, GameOverReasonFullWhenNoEmptyCellsRemain) {
    CubeBoard board;
    board.clear();
    for (int x = 0; x < BOARD_SIZE; ++x)
        for (int y = 0; y < BOARD_SIZE; ++y)
            for (int z = 0; z < BOARD_SIZE; ++z)
                board.set_for_testing(x, y, z, BLACK);

    EXPECT_EQ(board.game_over_reason(), GameOverReason::Full);
}

TEST(CubeBoard, GameOverReasonNoMovesWhenNeitherPlayerCanMove) {
    CubeBoard board;
    board.clear();
    // A lone Black stone with the rest empty: no sandwich possible for
    // either color anywhere on the board.
    board.set_for_testing(0, 0, 0, BLACK);

    EXPECT_FALSE(board.has_valid_moves(BLACK));
    EXPECT_FALSE(board.has_valid_moves(WHITE));
    EXPECT_EQ(board.game_over_reason(), GameOverReason::NoMoves);
}

TEST(CubeBoard, EvaluateIsBlackMinusWhiteCount) {
    CubeBoard board;
    EXPECT_EQ(board.evaluate(), board.count(BLACK) - board.count(WHITE));
    EXPECT_EQ(board.evaluate(), 0); // opening position is balanced
}
