"""
3D オセロ - GUI 表示モジュール（tkinter Canvas）

等角投影により 8×8×8 の立方体盤面を 2D で描画する。
面の奥行き表現：前面は明るい、後面は暗いグラデーションで表現。
"""

import tkinter as tk
from typing import Optional, List


class CubeOthelloGUI:
    """3D オセロの GUI（tkinter Canvas）"""

    def __init__(self, board, game_engine=None):
        self.board = board
        self.game_engine = game_engine  # ゲームエンジンへの参照
        self.root = tk.Tk()
        self.root.title("Cube Othello - 3D")
        self.root.geometry("800x700")

        # キャンバスの設定
        self.canvas_width = 760
        self.canvas_height = 540
        self.canvas = tk.Canvas(self.root, width=self.canvas_width, height=self.canvas_height, bg='#1a1a2e')
        self.canvas.pack(pady=10)

        # UI 要素：インフォパネル
        self.info_frame = tk.Frame(self.root, bg='#3a3a4e', bd=2, relief='raised')
        self.info_frame.pack(side='bottom', fill='x', padx=5, pady=5)

        # スコア表示
        self.score_label = tk.Label(
            self.info_frame, text="黒：0   白：0", fg='white', bg='#3a3a4e', font=('Consolas', 16))
        self.score_label.pack(side='left', padx=20)

        # ターン表示
        self.turn_label = tk.Label(
            self.info_frame, text="黒の番", fg='yellow', bg='#3a3a4e', font=('Consolas', 14))
        self.turn_label.pack(side='right')

        # ゲーム終了メッセージ
        self.message_label = tk.Label(
            self.info_frame, text="", fg='white', bg='#3a3a4e', font=('Consolas', 12), wraplength=500)
        self.message_label.pack(side='left', fill='both', expand=True, padx=(20, 0))

        # 初期描画
        self._draw()

    def _project(self, x: int, y: int, z: int) -> tuple[float, float]:
        """
        3D 座標 (x,y,z) を等角投影で 2D 平面上にプロジェクションする。

        等角投影の変換：
            screen_x = (x - y) * scale + center_x
            screen_y = (z + x + y) * scale / 2 + center_y

        ただし、今回は「立方体の各面」を独立して描画するため、
        面の中心座標に対して局部な 2D グリッドを描くアプローチをとる。

        今回は簡易的な表示として：
          - z が小さい（前面）→ y は下側、z が大きい（後面）→ y は上側
          - x の増加方向は右下がり
          - y の増加方向は左上がり
        という「斜め上からの視点」で表現する。

        変換：
            screen_x = (x - z) * scale + center_x
            screen_y = -(y + z) * scale / 2 + center_y
        """
        scale = 45.0  # スケール係数（ピクセル毎）
        center_x = self.canvas_width // 2
        center_y = self.canvas_height - 60

        screen_x = (x - z) * scale + center_x
        screen_y = -(y + z) * scale / 2 + center_y

        return screen_x, screen_y

    def _draw_cube_frame(self):
        """立方体の枠を描画（8 つの面）"""
        self.canvas.delete("all")

        # 面の中心座標を計算
        face_centers = {}
        for face_name in ['front', 'back', 'left', 'right', 'top', 'bottom']:
            if face_name == 'front':   # z=0 の面（前面）
                cx, cy = (7 * 45 + 23), -(8 * 45 / 2) + self.canvas_height - 60
            elif face_name == 'back':   # z=7 の面（後面）
                cx, cy = (-7 * 45 + 23), -(8 * 45 / 2) + self.canvas_height - 60
            elif face_name == 'left':   # x=0 の面（左側面）
                cx, cy = (1 * 45), -(7 * 45 / 2) + self.canvas_height - 60
            elif face_name == 'right':  # x=7 の面（右側面）
                cx, cy = (-7 * 45 + 7 * 45 + 1), -(7 * 45 / 2) + self.canvas_height - 60
            elif face_name == 'top':    # y=0 の面（上面）
                cx, cy = (7 * 45 + 36), -(8 * 45 / 2) + 12
            elif face_name == 'bottom': # y=7 の面（下面）
                cx, cy = (7 * 45 + 36), -((0 + 7) * 45 / 2) + self.canvas_height - 60

            face_centers[face_name] = (cx, cy)

        # 各面の枠を描画（奥行きによる色変化）
        colors = {
            'front': '#ffffff',
            'back':  '#aaaaaa',
            'left':  '#dddddd',
            'right': '#cccccc',
            'top':   '#eeeee',
            'bottom':'#bbbbbb'
        }

        for face_name in ['front', 'back', 'left', 'right', 'top', 'bottom']:
            cx, cy = face_centers[face_name]
            color = colors[face_name]

            # 面内のグリッドの枠を描く
            grid_color = f"#{int(float('#'+color.replace('#',''))/255*180):02x}"

            for i in range(7):
                x1, y1 = (i + 1) * 45 - 36, cy - 180 - (i + 1) * 45 / 2
                x2, y2 = (i + 2) * 45 - 36, cy - 180 - (i + 2) * 45 / 2

                # 前面と後面は 1 ピクセルの線、側面は太い線
                width = 2 if face_name in ('front', 'back') else 1
                self.canvas.create_line(x1, y1 - 38 + 60, x2, y2 - 38 + 60,
                                        fill=grid_color, width=width)

            # 水平方向の枠（x 軸方向）
            for j in range(7):
                base_y = cy - 180 - (j + 1) * 45 / 2
                x_start_left = 36
                x_end_right = cx - 36

                # 左側面（x=0）から右側面（x=7）へ
                for i in range(7):
                    x1, y1 = (i + 1) * 45 - 36, base_y - 180 - (j + 1) * 45 / 2
                    x2, y2 = (i + 2) * 45 - 36, base_y - 180 - (j + 2) * 45 / 2

                    width = 2 if face_name in ('front', 'back') else 1
                    self.canvas.create_line(x1, y1, x2, y2, fill=grid_color, width=width)

        # メインの枠（外側の境界）を描画
        for i in range(8):
            x1 = (i + 1) * 45 - 36
            y1 = -(i + 1) * 45 / 2 + self.canvas_height - 60 - 90

            # 前面（z=0）の枠
            x2, y2 = x1 + 45, y1 - 45
            self.canvas.create_line(x1, y1, x2, y2, fill='#ffffff', width=3)

        # 右側面の枠（x=7 の面）
        for i in range(8):
            base_y = -(i + 1) * 45 / 2 + self.canvas_height - 60
            x = (i + 1) * 45 - 36
            y = base_y - 90
            self.canvas.create_line(x, y, x - 72, y - 72, fill='#cccccc', width=3)

        # 後面（z=7）の枠
        for i in range(8):
            x1 = (i + 1) * 45 - 36
            y1 = -(i + 1) * 45 / 2 + self.canvas_height - 60 - 90

            x2, y2 = x1 - 72, y1 - 72
            self.canvas.create_line(x1, y1, x2, y2, fill='#aaaaaa', width=3)

    def _draw_pieces(self):
        """石（黒と白）を描画"""
        # すべてのマスをチェックして石がある場合を描画
        for x in range(8):
            for y in range(8):
                for z in range(8):
                    color = self.board.get_cell(x, y, z)
                    if color != '.':
                        screen_x, screen_y = self._project(x, y, z)

                        # 面の奥行きによる色変化（前面：明るい、後面：暗い）
                        surface_color = color

                        # 石の描画（円形）
                        radius = 8
                        if color == 'B':
                            # 黒石（濃いグレーからさらに暗く）
                            self.canvas.create_oval(
                                screen_x - radius, screen_y - radius,
                                screen_x + radius, screen_y + radius,
                                fill='#1a1a2e', outline='white'
                            )
                        else:  # color == 'W'
                            # 白石（白色から薄いグレーのグラデーション）
                            grad = self._get_white_gradient(z)
                            self.canvas.create_oval(
                                screen_x - radius, screen_y - radius,
                                screen_x + radius, screen_y + radius,
                                fill=grad[0], outline='white', width=1
                            )

                        # 石の中心に小さな点（立体感のため）
                        self.canvas.create_oval(
                            screen_x - radius//2, screen_y - radius//2,
                            screen_x + radius//2, screen_y + radius//2,
                            fill='white', width=1
                        )

        # 挟み込める手の表示（「+」マーク）
        if self.game_engine:
            valid_moves = self.game_engine.get_valid_moves_for_current_player()
            for (x, y, z) in valid_moves[:20]:  # 最大 20 マスまで表示
                screen_x, screen_y = self._project(x, y, z)

                # 文字「+」を描画（簡易）
                self.canvas.create_text(
                    screen_x - 5, screen_y + 3, text='+', fill='#ffffaa',
                    font=('Consolas', 10), anchor='center'
                )

    def _get_white_gradient(self, z: int) -> str:
        """z 座標に応じて白のグラデーション色を取得"""
        # z が大きい（奥）ほど暗く、小さい（手前）ほど明るい
        base = '#ffffff'
        if z >= 4:
            return '#f0f0e8'   # 奥 → 薄灰色
        elif z >= 2:
            return '#fafafa'
        else:
            return '#ffffff'

    def _update_display(self):
        """盤面と情報を更新"""
        self._draw_cube_frame()
        self._draw_pieces()

        # スコア表示
        counts = self.board.count_pieces()
        self.score_label.config(text=f"黒：{counts['B']}   白：{counts['W']}")

        # ターン表示
        current_player = '黒' if np.random.rand() < 0.5 else '白'
        self.turn_label.config(text=current_player + "の番")

    def play(self):
        """tkinter メインループを起動"""
        self.root.mainloop()


# numpy をここでインポート（型ヒントのため）
import numpy as np
