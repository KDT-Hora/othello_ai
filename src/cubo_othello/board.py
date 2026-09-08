"""
Cube Othello Board Module — 8×8×8 の立方体盤面管理と挟み込み判定。

C++ 移植用 Python プロトタイプ。すべてのロジックは C++ と同義に実装済み。

機能:
- 8×8×8 のグリッド（x,y,z = 0..7）
- 初期配置：(3,3)〜(4,4)の中心部で黒・白を隣り合うように配置
- 6 方向（±x, ±y, ±z）の挟み込み判定（斜めは考慮しない）
- 有効な手の取得、石数カウント、ゲーム状態管理

注: Python はプロトタイピング用であり、C++ の実装が本番コードです。
"""

import random
from typing import List, Tuple, Optional


BOARD_SIZE = 8
BLACK = 'B'
WHITE = 'W'
EMPTY = '.'


class CubeBoard:
    """Cube Othello の盤面管理クラス（8×8×8）。

    Attributes:
        grid: 512 マスの 3D グリッド（x,y,z = 0..7）
    """

    def __init__(self) -> None:
        self.grid: List[List[List[str]]] = [
            [[EMPTY for _ in range(BOARD_SIZE)] for _ in range(BOARD_SIZE)]
            for _ in range(BOARD_SIZE)
        ]
        self._set_initial_pieces()

    def _set_initial_pieces(self) -> None:
        """立方体の中心部（4×4×2 の真ん中）に黒・白を隣り合うように配置。

        座標系：x(左→右), y(上→下), z(前→後)

        初期配置の位置:
            (3,3,3), (3,3,4), (3,4,3), (3,4,4) → 黒（B）
            (4,3,3), (4,3,4), (4,4,3), (4,4,4) → 白（W）

        この配置により、両プレイヤーから挟み込みが可能になります。
        """
        # 黒石（左下奥側：x=3 の面）
        self.grid[3][3][3] = BLACK
        self.grid[3][3][4] = BLACK
        self.grid[3][4][3] = BLACK
        self.grid[3][4][4] = BLACK

        # 白石（右上手前側：x=4 の面）
        self.grid[4][3][3] = WHITE
        self.grid[4][3][4] = WHITE
        self.grid[4][4][3] = WHITE
        self.grid[4][4][4] = WHITE

    def get_cell(self, x: int, y: int, z: int) -> str:
        """指定座標のマス状態を取得。"""
        if 0 <= x < BOARD_SIZE and 0 <= y < BOARD_SIZE and 0 <= z < BOARD_SIZE:
            return self.grid[x][y][z]
        return EMPTY

    def set_cell(self, x: int, y: int, z: int, color: str) -> bool:
        """指定マスの色を設定（空いているか確認）"""
        if not (0 <= x < BOARD_SIZE and 0 <= y < BOARD_SIZE and 0 <= z < BOARD_SIZE):
            return False
        if self.grid[x][y][z] == EMPTY:
            self.grid[x][y][z] = color
            return True
        return False

    def flip_stones(self, x: int, y: int, z: int, player_color: str) -> List[Tuple[int, int, int]]:
        """
        指定したマスをプレイヤーの色で置き、挟んだ相手の石をひっくり返す。

        Args:
            x, y, z: 盤面の座標（0〜7）
            player_color: 自分の色（'B' または 'W'）

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

            # 相手の色の石が連続しているか確認（挟み込みの「挟まれた部分」）
            opponent = WHITE if player_color == BLACK else BLACK

            while (0 <= rx < BOARD_SIZE and 0 <= ry < BOARD_SIZE and 0 <= rz < BOARD_SIZE
                   and self.grid[rx][ry][rz] == opponent):
                rx += dx
                ry += dy
                rz += dz

            # 挟み込みが成立するか確認（挟んだ先頭に自分の色があるか）
            if (0 <= rx < BOARD_SIZE and 0 <= ry < BOARD_SIZE and 0 <= rz < BOARD_SIZE
                    and self.grid[rx][ry][rz] == player_color):
                # 挟み込み成功：挟まれた相手の石をひっくり返す
                cx, cy, cz = x + dx, y + dy, z + dz
                while not (self.grid[cx][cy][cz] == player_color or self.grid[cx][cy][cz] == EMPTY):
                    flipped.append((cx, cy, cz))
                    self.grid[cx][cy][cz] = player_color
                    cx += dx
                    cy += dy
                    cz += dz

        return flipped

    def place_piece(self, x: int, y: int, z: int) -> List[Tuple[int, int, int]]:
        """指定マスの石を置き、ひっくり返す処理を一括で実行。

        黒から開始（通常ルール：黒が先攻）。ランダムに色を選択する。

        Args:
            x, y, z: 盤面の座標（0〜7）

        Returns:
            ひっくり返した石のリスト（空なら無効な手）
        """
        if self.grid[x][y][z] != EMPTY:
            return []

        # 黒から開始（通常ルール：黒が先攻）— ランダムで切り替える
        player_color = BLACK if random.random() < 0.5 else WHITE
        flipped = self.flip_stones(x, y, z, player_color)

        # 自陣の石を配置
        self.grid[x][y][z] = player_color

        return flipped

    def count_pieces(self) -> dict:
        """各プレイヤーの石数をカウント"""
        counts = {'B': 0, 'W': 0}
        for x in range(BOARD_SIZE):
            for y in range(BOARD_SIZE):
                for z in range(BOARD_SIZE):
                    val = self.grid[x][y][z]
                    if val == BLACK:
                        counts['B'] += 1
                    elif val == WHITE:
                        counts['W'] += 1
        return counts

    def count_flippable(self, x: int, y: int, z: int) -> int:
        """指定マスを配置したときにひっくり返せる枚数をカウント"""
        flipped = self.flip_stones(x, y, z, BLACK)
        return len(flipped)

    def get_valid_moves(self) -> List[Tuple[int, int, int]]:
        """置ける手のリストを取得（双方から有効な手があるか）"""
        valid = []
        for x in range(BOARD_SIZE):
            for y in range(BOARD_SIZE):
                for z in range(BOARD_SIZE):
                    if self.grid[x][y][z] == EMPTY:
                        flipped_black = self.flip_stones(x, y, z, BLACK)
                        flipped_white = self.flip_stones(x, y, z, WHITE)

                        # 黒が置ける手、または白が置ける手のどちらか
                        if len(flipped_black) > 0 or len(flipped_white) > 0:
                            valid.append((x, y, z))
        return valid

    def get_valid_moves_for_color(self, color: str) -> List[Tuple[int, int, int]]:
        """指定色のプレイヤーが置ける手のリストを取得"""
        valid = []
        for x in range(BOARD_SIZE):
            for y in range(BOARD_SIZE):
                for z in range(BOARD_SIZE):
                    if self.grid[x][y][z] == EMPTY:
                        flipped = self.flip_stones(x, y, z, color)
                        if len(flipped) > 0:
                            valid.append((x, y, z))
        return valid

    def is_full(self) -> bool:
        """盤面が完全に埋まっているか"""
        for x in range(BOARD_SIZE):
            for y in range(BOARD_SIZE):
                for z in range(BOARD_SIZE):
                    if self.grid[x][y][z] == EMPTY:
                        return False
        return True

    def get_board_state(self) -> List[List[List[str]]]:
        """盤面の状態を返す（表示用）"""
        import copy
        result = [row[:] for row in [col[:] for col in self.grid]]
        return result


if __name__ == '__main__':
    # --- 動作確認テスト ---
    board = CubeBoard()

    print("=" * 50)
    print("Cube Othello Board — 初期状態（Python プロトタイプ）")
    print("=" * 50)

    counts = board.count_pieces()
    print(f"石の数：黒={counts['B']}, 白={counts['W']}")

    valid_moves = board.get_valid_moves_for_color('B')
    print(f"\n黒の置ける手（{len(valid_moves)}個）: {valid_moves}")

    # 有効な手に配置して動作確認
    if valid_moves:
        x, y, z = valid_moves[0]
        flipped = board.place_piece(x, y, z)
        print(f"\n({x},{y},{z}) に黒を置いた → ひっくり返り：{len(flipped)} 個")

    counts_after = board.count_pieces()
    print(f"配置後：黒={counts_after['B']}, 白={counts_after['W']}")
