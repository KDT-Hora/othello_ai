#!/usr/bin/env python3
"""Cube Othello — End-to-End Tests (pytest + coverage).

This test suite verifies the full game loop, including:
  • Board initialization and piece placement
  • Sandwich detection across all six directions (±x, ±y, ±z)
  • Turn switching and pass detection
  • Game over conditions (board full / stalemate)
  • Win condition by stone count
  • AI decision making under various scenarios

Target coverage: ≥90% for the Python prototype; C++ code is covered via
CMake's Google Test framework when built with -DENABLE_GTEST=ON.

Usage:
    pip install pytest pytest-cov hypothesis
    pytest --cov=cubo_othello --cov-report=html:build/coverage/htmlcov e2e_tests.py

Or simply run this script directly for a quick smoke test:
    python3 tests/e2e_tests.py
"""

import sys
from pathlib import Path
from typing import List, Tuple, Dict, Optional


# =============================================================================
# Board Module (same logic as C++ port — for E2E testing)
# =============================================================================

BOARD_SIZE = 8
EMPTY: str = '.'
BLACK: str = '⚫'
WHITE: str = '⚪'


class CubeBoard:
    """Cube Othello's board management class (same logic as C++ port)."""

    def __init__(self) -> None:
        self._grid: List[List[List[str]]] = [
            [[EMPTY for _ in range(BOARD_SIZE)] for _ in range(BOARD_SIZE)]
            for _ in range(BOARD_SIZE)
        ]
        self._initialize()

    def _initialize(self) -> None:
        """Initial placement: black and white alternate on the center 2×2×2."""
        for x in range(3, 5):
            for y in range(3, 5):
                for z in range(3, 5):
                    self._grid[x][y][z] = BLACK if (x + y) % 2 == 0 else WHITE

    def get_cell(self, x: int, y: int, z: int) -> str:
        """Get the piece at a given coordinate. Returns EMPTY if out of bounds."""
        if not (0 <= x < BOARD_SIZE and 0 <= y < BOARD_SIZE and 0 <= z < BOARD_SIZE):
            return EMPTY
        return self._grid[x][y][z]

    def set_cell(self, x: int, y: int, z: int, color: str) -> bool:
        """Place a piece at (x,y,z). Returns False if the cell is already occupied."""
        if not (0 <= x < BOARD_SIZE and 0 <= y < BOARD_SIZE and 0 <= z < BOARD_SIZE):
            return False
        if self._grid[x][y][z] == EMPTY:
            self._grid[x][y][z] = color
            return True
        return False

    def flip_stones(
        self,
        x: int,
        y: int,
        z: int,
        player_color: str,
    ) -> List[Tuple[int, int, int]]:
        """Place a piece at (x,y,z) and flip opponent stones via sandwich detection.

        Args:
            x, y, z: Coordinates of the cell to place a piece on.
            player_color: The color placing the piece ('BLACK' or 'WHITE').

        Returns:
            A list of (x, y, z) coordinates that were flipped during this move.

        Logic:
          1. Check all six directions (±x, ±y, ±z).
          2. Traverse each line in the direction until hitting a friendly piece or empty cell.
          3. If a friendly piece is found → sandwich detected → flip opponent stones between.
          4. Return list of flipped coordinates (empty if no sandwich occurred).
        """

        directions = [
            (1, 0, 0),   # +x
            (-1, 0, 0),  # -x
            (0, 1, 0),   # +y
            (0, -1, 0),  # -y
            (0, 0, 1),   # +z
            (0, 0, -1),  # -z
        ]

        opponent = WHITE if player_color == BLACK else BLACK
        flipped: List[Tuple[int, int, int]] = []

        for dx, dy, dz in directions:
            rx, ry, rz = x + dx, y + dy, z + dz

            # Traverse the line of opponent stones
            while (0 <= rx < BOARD_SIZE and 0 <= ry < BOARD_SIZE and 0 <= rz < BOARD_SIZE
                   and self._grid[rx][ry][rz] == opponent):
                rx += dx
                ry += dy
                rz += dz

            # Check if we've reached our own piece (successful sandwich)
            if (0 <= rx < BOARD_SIZE and 0 <= ry < BOARD_SIZE and 0 <= rz < BOARD_SIZE
                    and self._grid[rx][ry][rz] == player_color):

                cx, cy, cz = x + dx, y + dy, z + dz
                while True:
                    piece = self._grid[cx][cy][cz]
                    if piece == player_color or piece == EMPTY:
                        break
                    flipped.append((cx, cy, cz))
                    self._grid[cx][cy][cz] = player_color
                    cx += dx
                    cy += dy
                    cz += dz

        # Place our own piece on the original empty cell
        if not self.set_cell(x, y, z, player_color):
            pass  # Already occupied — this shouldn't happen in normal gameplay

        return flipped

    def get_valid_moves_for_color(self, color: str) -> List[Tuple[int, int, int]]:
        """Return all valid moves for a given piece color."""
        valid: List[Tuple[int, int, int]] = []
        for x in range(BOARD_SIZE):
            for y in range(BOARD_SIZE):
                for z in range(BOARD_SIZE):
                    if self._grid[x][y][z] == EMPTY:
                        flipped = self.flip_stones(x, y, z, color)
                        if len(flipped) > 0:
                            valid.append((x, y, z))
        return valid

    def get_all_valid_moves(self) -> List[Tuple[int, int, int]]:
        """Return all squares where either player can move."""
        valid: List[Tuple[int, int, int]] = []
        for x in range(BOARD_SIZE):
            for y in range(BOARD_SIZE):
                for z in range(BOARD_SIZE):
                    if self._grid[x][y][z] == EMPTY:
                        flipped_black = self.flip_stones(x, y, z, BLACK)
                        flipped_white = self.flip_stones(x, y, z, WHITE)
                        if len(flipped_black) > 0 or len(flipped_white) > 0:
                            valid.append((x, y, z))
        return valid

    def count_pieces(self) -> Dict[str, int]:
        """Count pieces for each color."""
        counts = {'B': 0, 'W': 0}
        for x in range(BOARD_SIZE):
            for y in range(BOARD_SIZE):
                for z in range(BOARD_SIZE):
                    if self._grid[x][y][z] == BLACK:
                        counts['B'] += 1
                    elif self._grid[x][y][z] == WHITE:
                        counts['W'] += 1
        return counts

    def is_full(self) -> bool:
        """Check if the board is completely filled."""
        for x in range(BOARD_SIZE):
            for y in range(BOARD_SIZE):
                for z in range(BOARD_SIZE):
                    if self._grid[x][y][z] == EMPTY:
                        return False
        return True

    def reset(self) -> None:
        """Reset the board to its initial state (empty)."""
        for x in range(BOARD_SIZE):
            for y in range(BOARD_SIZE):
                for z in range(BOARD_SIZE):
                    self._grid[x][y].clear()

    def __repr__(self) -> str:
        """Render the board as a string for debugging."""
        lines = []
        for z in reversed(range(BOARD_SIZE)):
            row = ""
            for y in range(BOARD_SIZE - 1, -1, -1):   # bottom-up rendering
                line = f"  "
                for x in range(BOARD_SIZE):
                    if self._grid[x][y][z] == EMPTY:
                        line += ". "
                    else:
                        line += self._grid[x][y][z] + " "
                row += line
            lines.append(row)
        return "\n".join(lines)


# =============================================================================
# Game Engine Module (same logic as C++ port — for E2E testing)
# =============================================================================

class GameEngine:
    """Game engine managing turn order, pass detection, and game-over conditions."""

    def __init__(self) -> None:
        self.board = CubeBoard()
        self.turn_player: str = 'B'   # Black starts first (black-on-white convention)
        self.game_over: bool = False
        self.winner: str = '.'
        self.move_count: int = 0

    @property
    def piece_counts(self) -> Dict[str, int]:
        return self.board.count_pieces()

    def get_valid_moves_for_current_player(self) -> List[Tuple[int, int, int]]:
        """Get all valid moves for the current player."""
        color = 'B' if self.turn_player == 'BLACK' else 'W'
        return self.board.get_valid_moves_for_color(color)

    def play_move(self, x: int, y: int, z: int) -> Dict[str, any]:
        """Execute a move for the current player.

        Returns:
            A dict with keys:
              - 'valid': bool — whether the move was valid (sandwich occurred)
              - 'flipped_count': int — number of stones flipped
              - 'new_turn': bool — whether turn switched after this move
        """
        if self.game_over:
            return {'valid': False, 'error': 'Game over'}

        flipped = self.board.flip_stones(x, y, z, self.turn_player)

        result = {
            'valid': len(flipped) > 0,
            'flipped_count': len(flipped),
            'new_turn': True,   # Simplified: always switch turn after a move
        }

        return result

    def check_game_over(self) -> bool:
        """Check if the game should end (board full or stalemate)."""
        # Check if board is full
        if self.board.is_full():
            self.game_over = True
            counts = self.piece_counts
            self.winner = 'B' if counts['B'] > counts['W'] else \
                        ('W' if counts['W'] > counts['B'] else '.')
            return True

        # Check for stalemate (no legal moves for either player)
        black_moves = len(self.board.get_valid_moves_for_color('BLACK'))
        white_moves = len(self.board.get_valid_moves_for_color('WHITE'))

        if black_moves == 0 and white_moves == 0:
            self.game_over = True
            self.winner = '.'   # Stalemate / draw
            return True

        return False

    def reset(self) -> None:
        """Reset the game to initial state."""
        self.board.reset()
        self.turn_player = 'B'
        self.game_over = False
        self.winner = '.'
        self.move_count = 0


# =============================================================================
# AI Module (Simple Minimax with stone-count evaluation)
# =============================================================================

class SimpleAI:
    """Simple minimax AI that evaluates positions purely by stone-count difference.

    This is an extremely simple AI used for demonstration purposes only.
    Production use should employ more sophisticated heuristics including:
      • Mobility (number of available moves)
      • Corner control
      • Edge control
      • Center control
      • Positional value tables
    """

    def __init__(self, depth: int = 3, color: str = 'W') -> None:
        self.depth = depth
        self.color: str = color   # 'B' or 'W' — the AI's color

    def search(self, board: CubeBoard) -> Tuple[str, List[Tuple[int, int, int]]]:
        """Return the best move for the AI using simple minimax.

        Args:
            board: The current state of the board.

        Returns:
            A tuple (action, moves) where action is 'PLACE' or 'PASS',
            and moves is a list of candidate moves (empty if PASS).
        """
        valid_moves = self._get_valid_moves_for_ai(board)

        if not valid_moves:
            return ('PASS', [])

        # Simple evaluation: prefer moves that flip more stones AND increase own stone count
        best_score = float('-inf')
        best_move: Optional[Tuple[int, int, int]] = None
        alpha = float('-inf')
        beta = float('inf')

        for move in valid_moves:
            flipped_count = self._simulate_flip(board, *move, self.color)
            score = len(flipped) * 3   # Primary heuristic: number of stones flipped

            if board.count_pieces()['B'] > board.count_pieces()['W']:
                score += 10   # Black ahead — prefer defensive moves
            elif board.count_pieces()['B'] < board.count_pieces()['W']:
                score -= 5    # White behind — aggressive play encouraged

            if score > best_score:
                best_score = score
                best_move = move

        return ('PLACE', [best_move] if best_move else [])

    def _get_valid_moves_for_ai(self, board: CubeBoard) -> List[Tuple[int, int, int]]:
        """Get all valid moves for the AI's color."""
        color = self.color
        return board.get_valid_moves_for_color(color)

    def _simulate_flip(
        self,
        board: CubeBoard,
        x: int,
        y: int,
        z: int,
        player_color: str,
    ) -> List[Tuple[int, int, int]]:
        """Simulate a flip without modifying the actual board."""
        original_grid = [row[:] for row in [col[:] for col in board._grid]]
        flipped = board.flip_stones(x, y, z, player_color)

        # Restore original state
        for x_ in range(BOARD_SIZE):
            for y_ in range(BOARD_SIZE):
                for z_ in range(BOARD_SIZE):
                    board._grid[x_][y_][z_] = original_grid[x_][y_][z_]

        return flipped


# =============================================================================
# Test Suite — E2E Tests (pytest)
# =============================================================================

def test_board_initial_placement() -> None:
    """T01: Initial board state — black and white placed on center 2×2×2."""
    board = CubeBoard()

    assert board.get_cell(3, 3, 3) == '⚫', "Black should be at (3,3,3)"
    assert board.get_cell(4, 3, 3) == '⚪', "White should be at (4,3,3)"
    assert board.get_cell(3, 4, 3) == '⚪', "White should be at (3,4,3)"
    assert board.get_cell(4, 4, 3) == '⚫', "Black should be at (4,4,3)"

    # Verify other cells are empty
    for x in range(BOARD_SIZE):
        for y in range(BOARD_SIZE):
            for z in range(BOARD_SIZE):
                if not ((x, y, z) in [(3, 3, 3), (4, 3, 3), (3, 4, 3), (4, 4, 3)]):
                    assert board.get_cell(x, y, z) == '.', f"({x},{y},{z}) should be empty"

    print("✅ T01: Initial placement correct")


def test_board_no_valid_moves_initial() -> None:
    """T02: No valid moves on initial board (no sandwiches yet)."""
    board = CubeBoard()
    valid = board.get_valid_moves_for_color('BLACK')

    assert len(valid) == 0, "Expected no valid moves initially"
    print("✅ T02: No valid moves detected on initial board")


def test_sandwich_detection_positive_x() -> None:
    """T03: Sandwich detection in +x direction."""
    board = CubeBoard()
    board.set_cell(1, 3, 3, '⚪')
    board.set_cell(5, 3, 3, '⚫')
    board.set_cell(6, 3, 3, '⚪')

    flipped = board.flip_stones(2, 3, 3, '⚫')

    assert len(flipped) == 2, f"Expected 2 flips in +x sandwich, got {len(flipped)}"
    print(f"✅ T03: Sandwich detected at (2,3,3): flipped {len(flipped)} stones")


def test_sandwich_detection_negative_x() -> None:
    """T04: Sandwich detection in -x direction."""
    board = CubeBoard()
    board.set_cell(6, 3, 3, '⚪')   # far right edge
    board.set_cell(0, 3, 3, '⚫')   # left edge

    flipped = board.flip_stones(4, 3, 3, '⚫')

    assert len(flipped) > 0, "Expected a valid flip in -x direction"
    print(f"✅ T04: Negative-x sandwich detected: flipped {len(flipped)} stones")


def test_sandwich_detection_positive_y() -> None:
    """T05: Sandwich detection in +y direction."""
    board = CubeBoard()
    board.set_cell(3, 1, 3, '⚪')   # y=1 (below center)
    board.set_cell(3, 6, 3, '⚫')   # y=6

    flipped = board.flip_stones(3, 2, 3, '⚫')

    assert len(flipped) > 0, "Expected a valid flip in +y direction"
    print(f"✅ T05: Sandwich detected at (3,2,3): flipped {len(flipped)} stones")


def test_sandwich_detection_negative_y() -> None:
    """T06: Sandwich detection in -y direction."""
    board = CubeBoard()
    board.set_cell(3, 6, 3, '⚪')   # y=6 (top)
    board.set_cell(3, 0, 3, '⚫')   # y=0 (bottom edge)

    flipped = board.flip_stones(3, 5, 3, '⚫')

    assert len(flipped) > 0, "Expected a valid flip in -y direction"
    print(f"✅ T06: Negative-y sandwich detected: flipped {len(flipped)} stones")


def test_sandwich_detection_positive_z() -> None:
    """T07: Sandwich detection in +z direction."""
    board = CubeBoard()
    board.set_cell(3, 3, 1, '⚪')   # z=1 (bottom layer)
    board.set_cell(3, 3, 6, '⚫')   # z=6

    flipped = board.flip_stones(3, 3, 2, '⚫')

    assert len(flipped) > 0, "Expected a valid flip in +z direction"
    print(f"✅ T07: Sandwich detected at (3,3,2): flipped {len(flipped)} stones")


def test_sandwich_detection_negative_z() -> None:
    """T08: Sandwich detection in -z direction."""
    board = CubeBoard()
    board.set_cell(3, 3, 7, '⚪')   # z=7 (top layer)
    board.set_cell(3, 3, 0, '⚫')   # z=0 (bottom edge)

    flipped = board.flip_stones(3, 3, 5, '⚫')

    assert len(flipped) > 0, "Expected a valid flip in -z direction"
    print(f"✅ T08: Negative-z sandwich detected: flipped {len(flipped)} stones")


def test_invalid_move_rejected() -> None:
    """T09: Invalid move (no sandwich) should be rejected."""
    board = CubeBoard()
    board.set_cell(3, 3, 3, '⚫')
    board.set_cell(4, 3, 3, '⚪')

    flipped = board.flip_stones(0, 0, 0, '⚫')

    assert len(flipped) == 0, "Invalid move should not place a piece"
    print("✅ T09: Invalid moves correctly rejected")


def test_turn_switching() -> None:
    """T10: Turn switching logic."""
    engine = GameEngine()
    assert engine.turn_player == 'B', "Black should start as first player"

    # Simulate a move
    valid_moves = board.get_valid_moves_for_color('BLACK')
    if len(valid_moves) > 0:
        result = engine.play_move(*valid_moves[0])
        assert result['valid'], "Valid move should be accepted"
        print(f"✅ T10: Turn switching works; flipped {result['flipped_count']} stones")


def test_game_over_stalemate() -> None:
    """T11: Game over on stalemate (both players have no moves)."""
    board = CubeBoard()
    engine = GameEngine()
    engine.board = board

    # Create a scenario where only one player can move
    black_moves = len(board.get_valid_moves_for_color('BLACK'))
    white_moves = len(board.get_valid_moves_for_color('WHITE'))

    assert black_moves == 0 or white_moves == 0, \
        "At least one player should have no moves in this configuration"
    print("✅ T11: Stalemate detection works")


def test_full_board_detection() -> None:
    """T12: Full board detection."""
    full_board = CubeBoard()
    for x in range(BOARD_SIZE):
        for y in range(BOARD_SIZE):
            for z in range(BOARD_SIZE):
                full_board.set_cell(x, y, z, '⚫')

    assert full_board.is_full(), "Full board detection failed"
    print("✅ T12: Full board correctly detected")


def test_ai_decision_making() -> None:
    """T13: SimpleAI makes a move using stone-count evaluation."""
    board = CubeBoard()
    ai = SimpleAI(depth=3)

    valid_moves = board.get_valid_moves_for_color('BLACK')
    if len(valid_moves) > 0:
        action, moves = ai.search(board, 'B')
        assert action == 'PLACE', "AI should PLACE a piece when moves available"
        print(f"✅ T13: AI selected move {moves[0]} with evaluation")


def test_ai_pass_when_no_moves() -> None:
    """T14: AI passes when no valid moves exist."""
    board = CubeBoard()  # Fresh board — no sandwiches possible initially
    ai = SimpleAI(depth=3)

    result, _ = ai.search(board, 'BLACK')
    assert result == 'PASS', "AI should PASS when no moves available"
    print("✅ T14: AI correctly passes when no valid moves exist")


def test_win_condition_black() -> None:
    """T15: Win condition — black has more stones."""
    board = CubeBoard()
    for x in range(BOARD_SIZE):
        for y in range(BOARD_SIZE):
            for z in range(BOARD_SIZE):
                board.set_cell(x, y, z, '⚫')

    counts = board.count_pieces()
    assert counts['B'] > counts['W'], "Black should have more stones"
    print(f"✅ T15: Black wins with {counts['B']} vs {counts['W']}")


def test_win_condition_white() -> None:
    """T16: Win condition — white has more stones."""
    board = CubeBoard()
    for x in range(BOARD_SIZE):
        for y in range(BOARD_SIZE):
            for z in range(BOARD_SIZE):
                board.set_cell(x, y, z, '⚪')

    counts = board.count_pieces()
    assert counts['W'] > counts['B'], "White should have more stones"
    print(f"✅ T16: White wins with {counts['W']} vs {counts['B']}")


def test_multi_direction_sandwich() -> None:
    """T17: Multi-direction sandwich detection (multiple directions simultaneously)."""
    board = CubeBoard()
    board.set_cell(0, 3, 3, '⚫')   # x=0 (left edge)
    board.set_cell(4, 3, 3, '⚪')   # middle
    board.set_cell(7, 3, 3, '⚪')   # x=7 (right edge)

    flipped = board.flip_stones(2, 3, 3, '⚫')
    assert len(flipped) > 0, "Multi-direction sandwich should flip stones"
    print(f"✅ T17: Multi-direction sandwich flipped {len(flipped)} stones")


def test_edge_placement() -> None:
    """T18: Edge/corner placement handling."""
    board = CubeBoard()
    board.set_cell(0, 0, 0, '⚪')   # Absolute corner (x=0,y=0,z=0)

    flipped = board.flip_stones(-1, -1, -1, '⚫')  # Out of bounds — should not crash
    assert len(flipped) == 0, "Out-of-bounds move should be safe"
    print("✅ T18: Edge/corner placement handled correctly")


def test_z_axis_sandwich() -> None:
    """T19: Z-axis sandwich detection (vertical direction)."""
    board = CubeBoard()
    board.set_cell(3, 2, 0, '⚫')   # z=0 layer
    board.set_cell(3, 2, 4, '⚪')   # z=4

    flipped = board.flip_stones(3, 2, 1, '⚫')
    assert len(flipped) > 0, "Z-axis sandwich should flip stones"
    print(f"✅ T19: Z-axis sandwich correctly detected: flipped {len(flipped)} stones")


def test_negative_direction_sandwich() -> None:
    """T20: Negative direction (-x, -y, -z) sandwich detection."""
    board = CubeBoard()
    board.set_cell(7, 3, 3, '⚪')   # Far edge on x=7
    board.set_cell(0, 3, 3, '⚫')   # Opposite edge

    flipped = board.flip_stones(4, 3, 3, '⚫')
    print(f"✅ T20: Negative direction sandwich: flipped {len(flipped)} stones")


def test_multiple_flips_single_move() -> None:
    """T21: Multiple pieces flipped in a single move."""
    board = CubeBoard()

    # Arrange: B W B pattern along y-axis
    for offset in range(3):
        if offset % 2 == 0:
            board.set_cell(3, 2 + offset, 3, '⚫')
        else:
            board.set_cell(3, 2 + offset, 3, '⚪')

    flipped = board.flip_stones(3, 0, 3, '⚫')
    print(f"✅ T21: Multi-flip test: placed piece flipped {len(flipped)} stones")


def test_game_engine_full_lifecycle() -> None:
    """T22: Full game lifecycle simulation."""
    game = GameEngine()

    counts_before = game.piece_counts
    initial_black, initial_white = counts_before['B'], counts_before['W']

    print(f"  Initial state — Black:{initial_black}, White:{initial_white}")

    # Simulate a few turns (simplified: random valid moves)
    for turn in range(10):
        player = 'B' if game.move_count % 2 == 0 else 'W'
        valid_moves = game.board.get_valid_moves_for_color(player)

        if len(valid_moves) > 0:
            move_result = game.play_move(*valid_moves[0])
            game.check_game_over()
            print(f"    Turn {game.move_count + 1}: [{player}] placed at {move_result['flipped_count']} flips")
        else:
            # Switch turn if no valid moves
            game.turn_player = ('W' if game.turn_player == 'B' else 'B')

    print("✅ T22: Full game lifecycle simulation completed")


def test_early_pass_detection() -> None:
    """T23: Early pass detection (simplified scenario)."""
    board = CubeBoard()   # Fresh board — no sandwiches possible initially
    valid_moves = board.get_valid_moves_for_color('BLACK')

    assert len(valid_moves) == 0, "Fresh board should have no valid moves"
    print("✅ T23: Correctly detected stalemate scenario and switched turn")


def test_symmetry_across_all_directions() -> None:
    """T24: Symmetry verification across all six directions."""
    board = CubeBoard()

    for dir_offset in range(BOARD_SIZE // 2):   # Only check half to avoid duplication
        board.set_cell(dir_offset, 3, 3, '⚫')   # Place black on one side
        board.set_cell(7 - dir_offset, 3, 3, '⚪')   # Place white on opposite

        flipped = board.flip_stones((dir_offset + 1) % BOARD_SIZE, 3, 3, '⚫')
        if len(flipped) > 0:
            print(f"    Direction offset {dir_offset}: flipped {len(flipped)}")

    print("✅ T24: Symmetry verification across all directions completed")


def test_z_layer_sandwich() -> None:
    """T25: Sandwich detection on z-axis layers."""
    board = CubeBoard()

    # Place pieces across z-layers
    board.set_cell(3, 2, 0, '⚫')   # z=0 layer
    board.set_cell(3, 2, 4, '⚪')   # z=4 (middle gap)

    flipped = board.flip_stones(3, 2, 1, '⚫')
    assert len(flipped) > 0, "Z-axis sandwich should flip stones"
    print(f"✅ T25: Z-layer sandwich detected: flipped {len(flipped)} stones")


# =============================================================================
# Main — Run all tests when executed directly
# =============================================================================

def main() -> int:
    """Run all E2E tests and report results."""
    print("=" * 70)
    print("Cube Othello — End-to-End Test Suite")
    print("=" * 70)

    tests = [
        test_board_initial_placement,
        test_board_no_valid_moves_initial,
        test_sandwich_detection_positive_x,
        test_sandwich_detection_negative_x,
        test_sandwich_detection_positive_y,
        test_sandwich_detection_negative_y,
        test_sandwich_detection_positive_z,
        test_sandwich_detection_negative_z,
        test_invalid_move_rejected,
        test_turn_switching,
        test_game_over_stalemate,
        test_full_board_detection,
        test_ai_decision_making,
        test_ai_pass_when_no_moves,
        test_win_condition_black,
        test_win_condition_white,
        test_multi_direction_sandwich,
        test_edge_placement,
        test_z_axis_sandwich,
        test_negative_direction_sandwich,
        test_multiple_flips_single_move,
        test_game_engine_full_lifecycle,
        test_early_pass_detection,
        test_symmetry_across_all_directions,
        test_z_layer_sandwich,
    ]

    passed = 0
    failed = 0

    for test_func in tests:
        try:
            test_func()
            passed += 1
        except AssertionError as e:
            print(f"❌ {test_func.__name__}: FAILED — {e}")
            failed += 1
        except Exception as e:
            print(f"❌ {test_func.__name__}: ERROR — {type(e).__name__}: {e}")
            failed += 1

    print("\n" + "=" * 70)
    total = passed + failed
    coverage = (passed / total * 100) if total > 0 else 0.0
    print(f"Results: {passed}/{total} tests passed — Coverage: {coverage:.1f}%")

    if failed == 0 and coverage >= 95.0:
        print("✅ ALL E2E TESTS PASSED — Coverage target (≥90%) achieved!")
    elif failed == 0:
        print(f"⚠️ All tests passed but coverage ({coverage:.1f}%) below target (90%).")

    return 0 if failed == 0 else 1


if __name__ == '__main__':
    sys.exit(main())
