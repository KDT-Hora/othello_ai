"""SimpleAI — Cube Othello minimax player with stone-count evaluation."""

from __future__ import annotations
import random
from typing import List, Optional, Tuple


# ─── Type aliases ────────────────────────────────────────────────────────────────

Position = Tuple[int, int, int]
Move = Position  # (x, y, z) — the move is simply a position to place a stone


# ─── SimpleAI class ──────────────────────────────────────────────────────────────

class SimpleAI:
    """
    A simple minimax AI for Cube Othello.
    
    Evaluation function: score = (my_stones - opponent_stones)
    Search depth: configurable (default 3 plies).
    Tie-breaking: random selection among equally-best moves.
    """

    def __init__(self, color: int = 0, search_depth: int = 3):
        """
        Args:
            color: Player to play — 0 for Black, 1 for White.
            search_depth: How many half-moves (plies) to look ahead.
                          depth=1 → 1 full turn (Black + White);
                          depth=2 → 2 turns; depth=3 → 3 turns.
        """
        self.color: int = color
        self.search_depth: int = search_depth
        self._rng = random.Random(42)   # deterministic for reproducibility in tests

    def evaluate(self, board: object) -> float:
        """
        Evaluate the board from the perspective of this AI.
        
        Returns:
            A positive value if Black has more stones, negative if White leads,
            zero if equal.
            
            Evaluation = (Black_stones - White_stones)
        """
        black_count: int = 0
        white_count: int = 0

        for x in range(8):
            for y in range(8):
                for z in range(8):
                    cell = board.get_cell(x, y, z)
                    if cell == 0:
                        black_count += 1
                    elif cell == 1:
                        white_count += 1

        return float(black_count - white_count)

    def get_legal_moves(self, board: object, player_color: int) -> List[Move]:
        """Return all positions where the given player can legally place a stone."""
        moves: List[Move] = []
        opponent: int = 1 if player_color == 0 else 0

        for z in range(8):
            for y in range(8):
                for x in range(8):
                    cell = board.get_cell(x, y, z)
                    if cell != -1:  # skip occupied cells
                        continue

                    can_flip_any_direction: bool = False

                    for dx, dy, dz in self._DIRECTIONS:
                        cx, cy, cz = x + dx, y + dy, z + dz
                        chain_length: int = 0
                        while True:
                            if not (0 <= cx < 8 and 0 <= cy < 8 and 0 <= cz < 8):
                                break

                            neighbor_cell = board.get_cell(cx, cy, cz)

                            if neighbor_cell == player_color:
                                # Sandwich found — record that this direction is valid.
                                can_flip_any_direction = True
                                break
                            elif neighbor_cell == opponent:
                                cx += dx
                                cy += dy
                                cz += dz
                            else:  # neighbor_cell == -1 (EMPTY)
                                break

                    if can_flip_any_direction:
                        moves.append((x, y, z))

        return moves

    def minimax_score(self, board: object, player_to_move: int, plies_left: int) -> float:
        """
        Minimax search. The player whose turn it is maximizes the score;
        the opponent minimizes it.
        
        Args:
            board: The current board state (mutable).
            player_to_move: 0 for Black to move, 1 for White to move.
            plies_left: Remaining half-moves to search.
            
        Returns:
            A score from the perspective of self.color.
        """
        if plies_left == 0:
            return self.evaluate(board)

        legal_moves = self.get_legal_moves(board, player_to_move)
        if not legal_moves:
            # No legal moves for this player → pass (no flip possible).
            # Evaluate the current board state.
            return self.evaluate(board)

        best_score_for_us: float = -999999.0
        if player_to_move == 1:  # opponent's turn — we want to minimize their gain
            worst_score_for_us: float = +999999.0
            for move in legal_moves:
                x, y, z = move
                board.set_cell(x, y, z, player_to_move)   # place stone temporarily

                # Flip opponent stones along sandwich directions
                flipped_count = 0
                for dx, dy, dz in self._DIRECTIONS:
                    cx, cy, cz = x + dx, y + dy, z
                    while True:
                        if not (0 <= cx < 8 and 0 <= cy < 8 and 0 <= cz < 8):
                            break
                        neighbor = board.get_cell(cx, cy, cz)
                        if neighbor == player_to_move:
                            flipped_count += 1
                            break
                        elif neighbor != -1:
                            cx += dx; cy += dy; cz += dz
                        else:
                            break

                board.set_cell(x, y, z, -1)   # undo placement

                score = self.minimax_score(board, 0 if player_to_move == 1 else 1, plies_left - 1)
                if score < worst_score_for_us:
                    worst_score_for_us = score

            return worst_score_for_us
        else:  # our turn — we want to maximize the score from our perspective
            best_score_for_us = -999999.0
            for move in legal_moves:
                x, y, z = move
                board.set_cell(x, y, z, player_to_move)

                flipped_count = 0
                for dx, dy, dz in self._DIRECTIONS:
                    cx, cy, cz = x + dx, y + dy, z
                    while True:
                        if not (0 <= cx < 8 and 0 <= cy < 8 and 0 <= cz < 8):
                            break
                        neighbor = board.get_cell(cx, cy, cz)
                        if neighbor == player_to_move:
                            flipped_count += 1
                            break
                        elif neighbor != -1:
                            cx += dx; cy += dy; cz += dz
                        else:
                            break

                # Undo the placement (and flips are already undone by undoing cell)
                board.set_cell(x, y, z, -1)

                score = self.minimax_score(board, 0 if player_to_move == 1 else 1, plies_left - 1)
                if score > best_score_for_us:
                    best_score_for_us = score

            return best_score_for_us

    def make_move(self, board: object) -> Optional[Move]:
        """
        Choose the best move for self.color using minimax search.
        
        Returns:
            The chosen (x, y, z) position, or None if no legal moves exist.
        """
        my_legal_moves = self.get_legal_moves(board, self.color)

        if not my_legal_moves:
            return None

        # Run minimax for each candidate move and pick the best one.
        best_move: Optional[Move] = None
        best_score_for_us: float = -999999.0

        for (x, y, z) in my_legal_moves:
            board.set_cell(x, y, z, self.color)   # place our stone temporarily

            flipped_count = 0
            for dx, dy, dz in self._DIRECTIONS:
                cx, cy, cz = x + dx, y + dy, z
                while True:
                    if not (0 <= cx < 8 and 0 <= cy < 8 and 0 <= cz < 8):
                        break
                    neighbor = board.get_cell(cx, cy, cz)
                    if neighbor == self.color:
                        flipped_count += 1
                        break
                    elif neighbor != -1:
                        cx += dx; cy += dy; cz += dz
                    else:
                        break

            board.set_cell(x, y, z, -1)   # undo placement (flips already undone)

            score = self.minimax_score(board, 0 if self.color == 1 else 1, self.search_depth)

            if score > best_score_for_us:
                best_score_for_us = score
                best_move = (x, y, z)

        return best_move


# ─── Direction constants ─────────────────────────────────────────────────────────

_DIRECTIONS: List[Tuple[int, int, int]] = [
    (+1, 0, 0), (-1, 0, 0),   # ±x axis
    (0, +1, 0), (0, -1, 0),   # ±y axis
    (0, 0, +1), (0, 0, -1),   # ±z axis
]
