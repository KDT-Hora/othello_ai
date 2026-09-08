"""
3D オセロ - ゲームエンジン

ゲームの状態管理、ターン交代、ゲーム終了判定などを担当する。
"""

import numpy as np
from typing import Optional, List, Tuple


class GameEngine:
    """3D オセロのゲームエンジン"""

    def __init__(self):
        self.board = None  # CubeBoard インスタンス
        self.turn_player = 'B'   # 現在のプレイヤー（'B':黒，'W':白）
        self.history: List[str] = []
        self.game_over = False
        self.winner: Optional[str] = None

    def reset(self) -> None:
        """ゲームをリセット"""
        from cubo_othello.board import CubeBoard, EMPTY, BLACK, WHITE
        self.board = CubeBoard()
        self.turn_player = 'B'   # 黒から開始（先攻）
        self.history = []
        self.game_over = False
        self.winner = None

    @property
    def piece_counts(self) -> dict:
        """各プレイヤーの石数"""
        return self.board.count_pieces()

    def get_valid_moves_for_current_player(self) -> List[Tuple[int, int, int]]:
        """現在のプレイヤーが置ける手のリストを取得"""
        color = 'B' if self.turn_player == 'B' else 'W'
        return self.board.get_valid_moves_for_color(color)

    def play_move(self, x: int, y: int, z: int) -> dict:
        """
        現在のプレイヤーが指定したマスの石を置く。

        Args:
            x, y, z: 盤面の座標（0〜7）

        Returns:
            {'valid': bool, 'flipped_count': int, 'effect_triggered': str | None}
        """
        if self.game_over:
            return {'valid': False, 'error': 'ゲームが終了しています'}

        # 石を置く処理
        flipped = self.board.place_piece(x, y, z)

        result = {
            'valid': True,
            'flipped_count': len(flipped),
            'effect_triggered': None,
        }

        if not flipped:
            result['valid'] = False
            result['error'] = f'({x},{y},{z}) は置ける手ではありません'
            return result

        # エフェクト判定（簡易：石数閾値）
        flipped_count = len(flipped)
        if flipped_count >= 7:
            result['effect_triggered'] = 'return_burst'   # ターンリセット
            self.turn_player = 'B' if self.board.get_cell(x, y, z) == 'B' else 'W'
        elif flipped_count >= 5:
            result['effect_triggered'] = 'wall_strike'     # 壁設置（簡易版：無効）
            # 簡易 AI の場合のみ使用（人間プレイでは表示のみ）

        if result['effect_triggered'] == 'return_burst':
            self.history.append(f"リターン・バースト発動！")
        elif result['effect_triggered'] == 'wall_strike':
            self.history.append("ウォール・ストライク発動（簡易版）")

        # ターン交代（エフェクトでターンがリセットされた場合は除外）
        if result['effect_triggered'] != 'return_burst':
            self.switch_turn()

        return result

    def switch_turn(self) -> None:
        """ターンを交代する"""
        self.turn_player = 'W' if self.turn_player == 'B' else 'B'
        self.history.append(f"{self.turn_player}の番")

    def check_game_over(self) -> bool:
        """ゲーム終了条件をチェック"""
        # 1. 盤面が埋まった
        if self.board.is_full:
            self.game_over = True
            return True

        # 2. どちらかのプレイヤーが置ける手がない（パス）
        black_moves = len(self.board.get_valid_moves_for_color('B'))
        white_moves = len(self.board.get_valid_moves_for_color('W'))

        if black_moves == 0 and white_moves == 0:
            self.game_over = True
            return True

        return False

    def get_winner(self) -> Optional[str]:
        """勝敗を判定"""
        counts = self.piece_counts

        if counts['B'] > counts['W']:
            return 'B'   # 黒の勝ち
        elif counts['W'] > counts['B']:
            return 'W'   # 白の勝ち
        else:
            return None   # ドロー

    def get_winner_name(self) -> str:
        """勝敗表示用の文字列を取得"""
        winner = self.get_winner()
        if winner == 'B':
            return "黒（先攻）"
        elif winner == 'W':
            return "白（後手）"
        else:
            return "引き分け"

    def get_status(self) -> dict:
        """現在のゲームステータスを取得"""
        counts = self.piece_counts
        black_moves = len(self.board.get_valid_moves_for_color('B'))
        white_moves = len(self.board.get_valid_moves_for_color('W'))

        return {
            'game_over': self.game_over,
            'turn_player': self.turn_player,
            'piece_counts': counts,
            'black_moves': black_moves,
            'white_moves': white_moves,
            'history': self.history[-10:],   # 最近の 10 件だけ
        }
