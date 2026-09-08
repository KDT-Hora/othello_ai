"""
3D オセロ（Cube Othello）- 盤面管理モジュール

8×8×8 の立方体盤面を管理し、挟み込み判定・石の配置などを扱う。
"""

import numpy as np
from typing import List, Tuple, Optional


BOARD_SIZE = 8
BLACK = 'B'
WHITE = 'W'
EMPTY = '.'


class CubeBoard:
    """8×8×8 の立方体盤面"""

    def __init__(self):
        # x, y, z が 0〜7 の 3D グリッド
        self.grid: np.ndarray = np.full((BOARD_SIZE, BOARD_SIZE, BOARD_SIZE), EMPTY, dtype=str)
        self._set_initial_pieces()

    def _set_initial_pieces(self) -> None:
        """
        初期配置：立方体の真ん中部分の 2×2×2 を黒・白で隣り合うように配置。
        
        選択した座標：(3,3,3), (3,3,4), (3,4,3), (3,4,4) に黒、
                       (4,3,3), (4,3,4), (4,4,3), (4,4,4) に白
        これにより黒と白が互いに隣接し、両方から挟み込みが可能になる。
        """
        # 黒石（左下奥側）
        self.grid[3, 3, 3] = BLACK
        self.grid[3, 3, 4] = BLACK
        self.grid[3, 4, 3] = BLACK
        self.grid[3, 4, 4] = BLACK

        # 白石（右上手前側）
        self.grid[4, 3, 3] = WHITE
        self.grid[4, 3, 4] = WHITE
        self.grid[4, 4, 3] = WHITE
        self.grid[4, 4, 4] = WHITE

    @property
    def is_full(self) -> bool:
        """盤面が完全に埋まっているか"""
        return not np.any(self.grid == EMPTY)

    def get_cell(self, x: int, y: int, z: int) -> str:
        """マスの状態を取得"""
        if 0 <= x < BOARD_SIZE and 0 <= y < BOARD_SIZE and 0 <= z < BOARD_SIZE:
            return self.grid[x, y, z]
        return EMPTY

    def set_cell(self, x: int, y: int, z: int, color: str) -> bool:
        """指定マスの色を設定（空いているか確認）"""
        if not (0 <= x < BOARD_SIZE and 0 <= y < BOARD_SIZE and 0 <= z < BOARD_SIZE):
            return False
        if self.grid[x, y, z] == EMPTY:
            self.grid[x, y, z] = color
            return True
        return False

    def flip_stones(self, x: int, y: int, z: int, player_color: str) -> List[Tuple[int, int, int]]:
        """
        指定したマスをプレイヤーの色で置き、挟んだ相手の石をひっくり返す。
        
        6 つの方向（＋x, −x, ＋y, −y, ＋z, −z）すべてで挟み込み判定を行う。
        斜め方向は考慮しない。

        Returns:
            ひっくり返した石の (x,y,z) のリスト
        """
        flipped = []
        directions = [
            (1, 0, 0), (-1, 0, 0),   # ±x 方向
            (0, 1, 0), (0, -1, 0),    # ±y 方向
            (0, 0, 1), (0, 0, -1)     # ±z 方向
        ]

        for dx, dy, dz in directions:
            rx, ry, rz = x + dx, y + dy, z + dz

            # 相手の色の石が連続しているか確認
            opponent = WHITE if player_color == BLACK else BLACK

            while (0 <= rx < BOARD_SIZE and 0 <= ry < BOARD_SIZE and 0 <= rz < BOARD_SIZE
                   and self.grid[rx, ry, rz] == opponent):
                rx += dx
                ry += dy
                rz += dz

            # 挟み込みが成立するか確認（挟んだ先頭に自分の色があるか）
            if (0 <= rx < BOARD_SIZE and 0 <= ry < BOARD_SIZE and 0 <= rz < BOARD_SIZE
                    and self.grid[rx, ry, rz] == player_color):
                # 挟み込み成功：挟まれた相手の石をひっくり返す
                cx, cy, cz = x + dx, y + dy, z + dz
                while not (self.grid[cx, cy, cz] == player_color or self.grid[cx, cy, cz] == EMPTY):
                    flipped.append((cx, cy, cz))
                    self.grid[cx, cy, cz] = player_color
                    cx += dx
                    cy += dy
                    cz += dz

        return flipped

    def place_piece(self, x: int, y: int, z: int) -> List[Tuple[int, int, int]]:
        """指定マスの石を置き、ひっくり返す処理を一括で実行"""
        if self.grid[x, y, z] != EMPTY:
            return []

        # 黒から開始（通常ルール：黒が先攻）
        player_color = BLACK if np.random.rand() < 0.5 else WHITE
        flipped = self.flip_stones(x, y, z, player_color)

        # 自陣の石を配置
        self.grid[x, y, z] = player_color

        return flipped

    def count_pieces(self) -> dict:
        """各プレイヤーの石数をカウント"""
        counts = {'B': 0, 'W': 0}
        for x in range(BOARD_SIZE):
            for y in range(BOARD_SIZE):
                for z in range(BOARD_SIZE):
                    val = self.grid[x, y, z]
                    if val == BLACK:
                        counts['B'] += 1
                    elif val == WHITE:
                        counts['W'] += 1
        return counts

    def count_flippable(self, x: int, y: int, z: int) -> int:
        """指定マスを配置したときにひっくり返せる枚数をカウント"""
        flipped = self.flip_stones(x, y, z, BLACK)
        # 黒なら黒、白なら白としてカウント（実際には空だから両方 0）
        return len(flipped)

    def get_valid_moves(self) -> List[Tuple[int, int, int]]:
        """置ける手のリストを取得"""
        valid = []
        for x in range(BOARD_SIZE):
            for y in range(BOARD_SIZE):
                for z in range(BOARD_SIZE):
                    if self.grid[x, y, z] == EMPTY:
                        flipped = self.flip_stones(x, y, z, BLACK)
                        if len(flipped) > 0:
                            valid.append((x, y, z))
        return valid

    def get_valid_moves_for_color(self, color: str) -> List[Tuple[int, int, int]]:
        """指定色のプレイヤーが置ける手のリストを取得"""
        valid = []
        for x in range(BOARD_SIZE):
            for y in range(BOARD_SIZE):
                for z in range(BOARD_SIZE):
                    if self.grid[x, y, z] == EMPTY:
                        flipped = self.flip_stones(x, y, z, color)
                        if len(flipped) > 0:
                            valid.append((x, y, z))
        return valid

    def get_board_state(self) -> np.ndarray:
        """盤面の状態を返す（表示用）"""
        return self.grid.copy()
