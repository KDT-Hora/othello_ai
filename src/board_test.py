"""Cube Othello — Board Module (C++ 移植用 Python プロトタイプ）。

このモジュールは C++ の実装と完全な同義のロジックを持ちます。
動作確認のためまず Python でテストし、その後 C++ に移植します。
"""

import random
from typing import List, Tuple


BOARD_SIZE = 8
BLACK = 'B'
WHITE = 'W'
EMPTY = '.'


class CubeBoard:
    """Cube Othello の盤面管理クラス（8×8×3）."""

    def __init__(self) -> None:
        self.grid: List[List[List[str]]] = [
            [['.' for _ in range(BOARD_SIZE)] for _ in range(BOARD_SIZE)]
            for _ in range(BOARD_SIZE)
        ]
        self._set_initial_pieces()

    def _set_initial_pieces(self) -> None:
        """立方体の中心部（4×4 の一部）に黒・白を隣り合うように配置."""
        black_x, white_x = 3, 4
        for y in range(3, 5):
            for z in range(3, 5):
                self.grid[black_x][y][z] = BLACK
                self.grid[white_x][y][z] = WHITE

    def get_cell(self, x: int, y: int, z: int) -> str:
        """盤面のマスを取得."""
        if 0 <= x < BOARD_SIZE and 0 <= y < BOARD_SIZE and 0 <= z < BOARD_SIZE:
            return self.grid[x][y][z]
        return EMPTY

    def set_cell(self, x: int, y: int, z: int, color: str) -> bool:
        """マスを指定色にセット."""
        if not (0 <= x < BOARD_SIZE and 0 <= y < BOARD_SIZE and 0 <= z < BOARD_SIZE):
            return False
        if self.grid[x][y][z] == EMPTY:
            self.grid[x][y][z] = color
            return True
        return False

    def flip_stones(self, x: int, y: int, z: int, player_color: str) -> List[Tuple[int, int, int]]:
        """指定マスを置き、挟み込み判定でひっくり返す."""
        flipped = []
        directions = [
            (1, 0, 0), (-1, 0, 0),   # ±x
            (0, 1, 0), (0, -1, 0),    # ±y
            (0, 0, 1), (0, 0, -1)     # ±z
        ]

        for dx, dy, dz in directions:
            rx, ry, rz = x + dx, y + dy, z + dz
            opponent = WHITE if player_color == BLACK else BLACK

            while (0 <= rx < BOARD_SIZE and 0 <= ry < BOARD_SIZE and 0 <= rz < BOARD_SIZE
                   and self.grid[rx][ry][rz] == opponent):
                rx += dx
                ry += dy
                rz += dz

            if (0 <= rx < BOARD_SIZE and 0 <= ry < BOARD_SIZE and 0 <= rz < BOARD_SIZE
                    and self.grid[rx][ry][rz] == player_color):
                cx, cy, cz = x + dx, y + dy, z + dz
                while not (self.grid[cx][cy][cz] == player_color or self.grid[cx][cy][cz] == EMPTY):
                    flipped.append((cx, cy, cz))
                    self.grid[cx][cy][cz] = player_color
                    cx += dx
                    cy += dy
                    cz += dz

        self.set_cell(x, y, z, player_color)
        return flipped

    def get_valid_moves(self, color: str) -> List[Tuple[int, int, int]]:
        """指定色の有効な手のリスト."""
        valid = []
        for x in range(BOARD_SIZE):
            for y in range(BOARD_SIZE):
                for z in range(BOARD_SIZE):
                    if self.grid[x][y][z] == EMPTY:
                        flipped = self.flip_stones(x, y, z, color)
                        if len(flipped) > 0:
                            valid.append((x, y, z))
        return valid

    def get_all_valid_moves(self) -> List[Tuple[int, int, int]]:
        """双方から有効な手のリスト."""
        valid = []
        for x in range(BOARD_SIZE):
            for y in range(BOARD_SIZE):
                for z in range(BOARD_SIZE):
                    if self.grid[x][y][z] == EMPTY:
                        flipped_b = self.flip_stones(x, y, z, BLACK)
                        flipped_w = self.flip_stones(x, y, z, WHITE)
                        if len(flipped_b) > 0 or len(flipped_w) > 0:
                            valid.append((x, y, z))
        return valid

    def count_pieces(self) -> dict:
        """各プレイヤーの石数をカウント."""
        counts = {'B': 0, 'W': 0}
        for x in range(BOARD_SIZE):
            for y in range(BOARD_SIZE):
                for z in range(BOARD_SIZE):
                    val = self.grid[x][y][z]
                    if val == BLACK: counts['B'] += 1
                    elif val == WHITE: counts['W'] += 1
        return counts


def main():
    print("=" * 50)
    print("Cube Othello — Board Module Test")
    print("=" * 50)

    board = CubeBoard()
    print(f"\n初期配置（黒={board.get_cell(3,3,3)}, 白={board.get_cell(4,3,3)}）")
    print("盤面の一部:")
    for y in range(BOARD_SIZE):
        row = ''
        for x in range(BOARD_SIZE):
            z0 = board.grid[x][y][0]
            z1 = board.grid[x][y][1] if BOARD_SIZE > 1 else '.'
            row += f"[{board.grid[x][y][z0]} {board.grid[x][y][1]}]" if y == 3 or y == 4 else '   '
        print(row)

    valid = board.get_all_valid_moves()
    print(f"\n有効な手：{len(valid)}個")
    for mv in valid[:5]:
        x, y, z = mv
        flipped = board.flip_stones(x, y, z, BLACK)
        print(f"  ({x},{y},{z}) → 黒が {len(flipped)} 個ひっくり返る")

    counts = board.count_pieces()
    print(f"\n石の数：黒={counts['B']}, 白={counts['W']}")


if __name__ == '__main__':
    main()
