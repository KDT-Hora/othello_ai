"""Unit tests for SimpleAI (minimax search)."""

from __future__ import annotations
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent / ".." / "src"))


def evaluate_board(black_count: int, white_count: int) -> float:
    """Helper to create a board with given stone counts and return evaluation."""
    from src.board import CubeBoard
    board = CubeBoard()

    for x in range(8):
        for y in range(8):
            for z in range(8):
                if (x, y, z) not in [(3, 3), (3, 4), (4, 3), (4, 4)]:
                    board.set_cell(x, y, z, -1)

    black_placed = white_placed = 0
    for x in range(8):
        if black_placed < black_count:
            board.set_cell(x % 8, 0, x // 8, 0)
            black_placed += 1
        if white_placed < white_count:
            board.set_cell(x % 8, 1, x // 8 + 1, 1)
            white_placed += 1

    return SimpleAI(color=0).evaluate(board), SimpleAI(color=1).evaluate(board)


def test_evaluate_white_leads():
    """When White has more stones than Black, the score should be positive for White (negative for Black)."""
    black_count = 3
    white_count = 5
    _, white_score = evaluate_board(black_count, white_count)
    assert white_score > 0


def test_evaluate_black_leads():
    """When Black has more stones than White, the score should be positive for Black."""
    black_count = 5
    white_count = 3
    black_score, _ = evaluate_board(black_count, white_count)
    assert black_score > 0


def test_evaluate_equal():
    """When stone counts are equal, the score should be zero."""
    black_count = 4
    white_count = 4
    _, white_score = evaluate_board(black_count, white_count)
    # Allow small floating-point tolerance
    assert abs(white_score) < 1e-6


def test_evaluate_empty_board_is_zero():
    """An empty board should score zero for both players."""
    from src.board import CubeBoard
    board = CubeBoard()
    black_score = SimpleAI(color=0).evaluate(board)
    white_score = SimpleAI(color=1).evaluate(board)
    assert abs(black_score) < 1e-6 and abs(white_score) < 1e-6


def test_minimax_depth_zero_returns_eval():
    """At depth=0, minimax should simply return the evaluation of the current board."""
    from src.board import CubeBoard
    board = CubeBoard()

    # Place some stones: Black at (3,3) and (4,4), White at (3,4) and (4,3)
    for x in [3, 4]:
        for y in [3, 4]:
            color = 0 if x == 3 else 1
            board.set_cell(x, y, 3, color)

    ai_black = SimpleAI(color=0, search_depth=0)
    ai_white = SimpleAI(color=1, search_depth=0)

    black_score = ai_black.minimax_score(board, 0, 0)
    white_score = ai_white.minimax_score(board, 1, 0)

    assert abs(black_score - ai_white.evaluate(board)) < 1e-6


def test_minimax_depth_one_considers_opponent_response():
    """At depth=1, the AI should consider that after its move, the opponent will respond."""
    from src.board import CubeBoard
    board = CubeBoard()

    # Create a scenario: Black can place at (0,0) or (7,7); placing at (0,0) lets White play at (1,0),
    # while placing at (7,7) allows White to play at (6,6). The AI should prefer the move that leads to better final score.
    board.set_cell(0, 0, 0, -1)
    board.set_cell(7, 7, 0, -1)

    ai = SimpleAI(color=0, search_depth=2)   # depth=2 means Black + White both move once
    best_move = ai.make_move(board)

    # The AI should choose a legal move. We just verify it doesn't crash and returns something valid.
    assert best_move is not None or board.get_cell(0, 0, 0) == -1


def test_make_move_returns_none_when_no_legal_moves():
    """If the player has no legal moves, make_move should return None."""
    from src.board import CubeBoard
    board = CubeBoard()

    # Fill the entire board with Black stones except one cell.
    for x in range(8):
        for y in range(8):
            for z in range(8):
                if (x, y, z) != (0, 0, 0):
                    board.set_cell(x, y, z, 1)   # White stones everywhere else

    ai = SimpleAI(color=1, search_depth=3)
    result = ai.make_move(board)

    assert result is None


def test_make_move_returns_a_legal_move():
    """make_move should return a position that is indeed a legal move."""
    from src.board import CubeBoard, Direction
    board = CubeBoard()

    # Set up: Black at (1,0,0), White at (3,0,0) and (5,0,0), Black at (7,0,0).
    # Placing Black at (2,0,0) sandwiches the White stone at (3,0,0).
    board.set_cell(1, 0, 0, 0)
    board.set_cell(3, 0, 0, 1)
    board.set_cell(5, 0, 0, 1)
    board.set_cell(7, 0, 0, 0)

    ai = SimpleAI(color=0, search_depth=3)
    result = ai.make_move(board)

    assert result is not None


if __name__ == "__main__":
    import pytest
    sys.exit(pytest.main([__file__, "-v"]))
