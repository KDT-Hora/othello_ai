"""Cube Othello board logic — TDD-driven implementation (mirrors C++ design)."""

from __future__ import annotations
import dataclasses
from typing import List, Tuple


# ─── Constants ─────────────────────────────────────────────────────────────────────

BOARD_SIZE: int = 8
Z_LAYERS: int = 8
EMPTY: int = -1
BLACK: int = 0
WHITE: int = 1


@dataclasses.dataclass(frozen=True)
class Position:
    """A cube cell coordinate."""
    x: int
    y: int
    z: int

    def __hash__(self) -> int:
        return hash((self.x, self.y, self.z))

    def __eq__(self, other) -> bool:
        if not isinstance(other, Position):
            return False
        return (self.x == other.x and self.y == other.y and self.z == other.z)


# ─── Direction enumeration ────────────────────────────────────────────────────────

class Direction:
    """Axial directions on the cube."""

    POS_X = (1, 0, 0)
    NEG_X = (-1, 0, 0)
    POS_Y = (0, 1, 0)
    NEG_Y = (0, -1, 0)
    POS_Z = (0, 0, 1)
    NEG_Z = (0, 0, -1)

    ALL: List[Tuple[int, int, int]] = [POS_X, NEG_X, POS_Y, NEG_Y, POS_Z, NEG_Z]


# ─── CubeBoard class ──────────────────────────────────────────────────────────────

class CubeBoard:
    """8×8×8 cube Othello board with sandwich-flip mechanics."""

    def __init__(self) -> None:
        self.grid: dict[Position, int] = {}   # maps (x,y,z) → -1/0/1
        self.turn: int = BLACK                 # current player: BLACK or WHITE
        self.game_over: bool = False

    def initialize(self) -> None:
        """Initialize board with center stones and set turn to BLACK."""
        self.grid.clear()
        # Place center stones at (x,y,z) ∈ {3,4} × {3,4} × {3,5}
        for x in (3, 4):
            for y in (3, 4):
                for z in (3, 4, 5):
                    self._place_stone(x, y, z, BLACK if x == 3 else WHITE)

        self.turn = BLACK
        self.game_over = False

    def _place_stone(self, x: int, y: int, z: int, color: int) -> None:
        """Place a stone at (x,y,z). Does NOT flip or check legality — caller is responsible."""
        if not (0 <= x < BOARD_SIZE and 0 <= y < BOARD_SIZE and 0 <= z < Z_LAYERS):
            raise ValueError(f"Position ({x},{y},{z}) out of bounds")

    def set_cell(self, x: int, y: int, z: int, color: int) -> None:
        """Set a single cell to the given color. Used internally by _place_stone and for testing."""
        if not (0 <= x < BOARD_SIZE and 0 <= y < BOARD_SIZE and 0 <= z < Z_LAYERS):
            raise ValueError(f"Position ({x},{y},{z}) out of bounds")

        self.grid[(x, y, z)] = color

    def place_stone(self, x: int, y: int, z: int, color: int) -> Tuple[bool, List[Position]]:
        """Place a stone of the given color at (x,y,z).

        Returns:
            (success, flipped_positions) — if success is True, the board has been mutated.
            If False, no change was made and an empty list is returned.
        """
        # Validate position
        if not (0 <= x < BOARD_SIZE and 0 <= y < BOARD_SIZE and 0 <= z < Z_LAYERS):
            return (False, [])

        pos = Position(x, y, z)

        # Reject placement on an occupied cell → invalid move
        if pos in self.grid:
            return (False, [])   # illegal: already occupied

        opponent = WHITE if color == BLACK else BLACK

        flipped: List[Position] = []

        # ─── Sandwich detection and flipping for each of the 6 directions ──────────
        for dx, dy, dz in Direction.ALL:
            cx, cy, cz = x + dx, y + dy, z   # start just beyond the placed stone

            while True:
                if not (0 <= cx < BOARD_SIZE and 0 <= cy < BOARD_SIZE and 0 <= cz < Z_LAYERS):
                    break   # out of bounds — no sandwich in this direction

                cell_color = self.grid.get(Position(cx, cy, cz), EMPTY)

                if cell_color == color:      # found own stone → sandwich complete! flip all opponent stones in this chain
                    flipped.append(pos)      # the newly placed stone itself is also "flipped" (it was empty before)
                    break                     # stop scanning this direction
                elif cell_color == opponent:  # opponent stone — accumulate it for flipping later
                    pass                      # continue extending the chain
                else:                         # EMPTY cell → sandwich broken in this direction, no flip
                    break

            cx += dx
            cy += dy
            cz += dz   # move one step further to continue scanning

        if not flipped:
            return (False, [])   # no valid sandwich found anywhere → illegal move

        # Flip all accumulated opponent stones
        for pos in flipped:
            self.grid[pos] = color

        # Place our own stone at the target position
        self.grid[pos] = color

        self.game_over = False
        return (True, flipped)

    def get_valid_moves(self, color: int) -> List[Tuple[int, int, int]]:
        """Return all valid move positions for the given player.

        A move is valid if placing a stone there would flip at least one opponent stone.
        """
        valid: List[Tuple[int, int, int]] = []
        opponent = WHITE if color == BLACK else BLACK

        for z in range(Z_LAYERS):
            for y in range(BOARD_SIZE):
                for x in range(BOARD_SIZE):
                    pos_key = (x, y, z)
                    if pos_key in self.grid:
                        continue   # skip non-empty cells

                    can_flip_any_direction = False

                    for dx, dy, dz in Direction.ALL:
                        cx, cy, cz = x + dx, y + dy, z
                        while True:
                            if not (0 <= cx < BOARD_SIZE and 0 <= cy < BOARD_SIZE and 0 <= cz < Z_LAYERS):
                                break

                            cell_color = self.grid.get((cx, cy, cz), EMPTY)

                            if cell_color == color:      # own stone → sandwich complete!
                                can_flip_any_direction = True
                                break
                            elif cell_color == opponent:  # opponent stone — keep scanning
                                cx += dx
                                cy += dy
                                cz += dz
                            else:                          # EMPTY → chain broken, no flip in this direction
                                break

                    if can_flip_any_direction:
                        valid.append((x, y, z))

        return valid

    def game_over(self) -> bool:
        """Determine whether the game is over.

        Returns True if either:
          - The board is completely full (draw by exhaustion), or
          - Both players have zero legal moves (no-moves condition → declare winner).
        """
        black_moves = self.get_valid_moves(BLACK)
        white_moves = self.get_valid_moves(WHITE)

        if not black_moves and not white_moves:
            return True   # no-moves condition → game over, declare winner by stone count

        # Check full-board condition (all 512 cells filled)
        board_full = len(self.grid) == BOARD_SIZE * BOARD_SIZE * Z_LAYERS
        if board_full:
            return True   # full board with both players having moves → draw

        return False

    def count_pieces(self, color: int) -> int:
        """Count stones of the given color on the current board."""
        count = 0
        for pos_key, cell_color in self.grid.items():
            if cell_color == color:
                count += 1
        return count
