"""
3D オセロ - シンプル AI プレイヤー

簡易的なミニマックスアルゴリズムを実装。
評価関数は「黒石数 − 白石数」のみを考慮し、挟み込み枚数を軽視する（仕様 Q10-A）。
深さ制限付きで探索時間を短く抑える。
"""

import random
from typing import List, Optional, Tuple


class SimpleAI:
    """簡易 AI プレイヤー"""

    def __init__(self, name: str = "SimpleAI", depth: int = 3):
        self.name = name
        self.depth = depth  # 探索深さ（枝刈りあり）

    def evaluate(self, board) -> float:
        """盤面を評価してスコアを返す"""
        counts = board.count_pieces()
        score = counts['B'] - counts['W']
        return score

    def get_valid_moves(self, board) -> List[Tuple[int, int, int]]:
        """有効な手のリストを取得（簡易版）"""
        valid = []
        for x in range(8):
            for y in range(8):
                for z in range(8):
                    if board.get_cell(x, y, z) == '.':
                        # 挟み込み判定（簡易：黒のみチェック）
                        flipped = board.flip_stones(x, y, z, 'B')
                        if len(flipped) > 0:
                            valid.append((x, y, z))
        return valid

    def search(self, board, is_maximizing: bool, depth: int) -> Optional[Tuple[int, int, int]]:
        """ミニマックス探索（深さ制限付き）"""
        if depth == 0:
            # 葉ノードでの評価
            scores = []
            for move in self.get_valid_moves(board):
                score = self.evaluate_after_move(board, move)
                scores.append((move, score))

            if not scores:
                return None

            if is_maximizing:
                best_move, best_score = max(scores, key=lambda x: x[1])
            else:
                best_move, best_score = min(scores, key=lambda x: x[1])
            return best_move

        valid_moves = self.get_valid_moves(board)
        if not valid_moves:
            return None

        # ランダムに手を混ぜる（同点の場合）
        random.shuffle(valid_moves)

        best_score = float('inf') if is_maximizing else float('-inf')
        best_move = None

        for move in valid_moves:
            # 簡易な移動シミュレーション
            flipped = board.flip_stones(move[0], move[1], move[2], 'B' if is_maximizing else 'W')
            board.set_cell(move[0], move[1], move[2], 'B' if is_maximizing else 'W')

            result = self.search(board, not is_maximizing, depth - 1)
            board.set_cell(move[0], move[1], move[2], '.')  # rollback

            if result:
                score = self.evaluate_after_move(board, result)
                if (is_maximizing and score > best_score) or \
                   (not is_maximizing and score < best_score):
                    best_score = score
                    best_move = move

        return best_move

    def evaluate_after_move(self, board: 'CubeBoard', move: Tuple[int, int, int]) -> float:
        """移動後の盤面を評価"""
        # 簡易なシミュレーション
        x, y, z = move
        flipped_count = len(board.flip_stones(x, y, z, 'B'))

        board.set_cell(x, y, z, 'B')

        counts = board.count_pieces()
        score = counts['B'] - counts['W'] + 0.5 * flipped_count

        board.set_cell(x, y, z, '.')  # rollback
        return score

    def play(self, board) -> Optional[Tuple[int, int, int]]:
        """AI の手を決定"""
        valid = self.get_valid_moves(board)
        if not valid:
            return None

        result = self.search(board, is_maximizing=True, depth=self.depth)
        return result


class HeuristicAI:
    """ヒューリスティック AI（石数＋挟み込みを考慮）"""

    def __init__(self, name: str = "HeuristicAI", weight_flip: float = 0.5):
        self.name = name
        self.weight_flip = weight_flip  # 挟み込み枚数の重み

    def evaluate(self, board) -> float:
        """盤面を評価"""
        counts = board.count_pieces()
        score = counts['B'] - counts['W']
        return score

    def get_valid_moves(self, board) -> List[Tuple[int, int, int]]:
        valid = []
        for x in range(8):
            for y in range(8):
                for z in range(8):
                    if board.get_cell(x, y, z) == '.':
                        flipped = board.flip_stones(x, y, z, 'B')
                        if len(flipped) > 0:
                            valid.append((x, y, z))
        return valid

    def search(self, board, is_maximizing: bool, depth: int) -> Optional[Tuple[int, int, int]]:
        """簡易なミニマックス（深さ制限付き）"""
        if depth == 0:
            scores = []
            for move in self.get_valid_moves(board):
                score = self.evaluate_after_move(board, move)
                scores.append((move, score))

            if not scores:
                return None

            best_move, best_score = max(scores, key=lambda x: x[1])
            return best_move

        valid_moves = self.get_valid_moves(board)
        random.shuffle(valid_moves)

        best_score = float('inf') if is_maximizing else float('-inf')
        best_move = None

        for move in valid_moves:
            flipped = board.flip_stones(move[0], move[1], move[2], 'B' if is_maximizing else 'W')
            board.set_cell(move[0], move[1], move[2], 'B' if is_maximizing else 'W')

            result = self.search(board, not is_maximizing, depth - 1)
            board.set_cell(move[0], move[1], move[2], '.')

            if result:
                score = self.evaluate_after_move(board, result)
                if (is_maximizing and score > best_score) or \
                   (not is_maximizing and score < best_score):
                    best_score = score
                    best_move = move

        return best_move

    def evaluate_after_move(self, board: 'CubeBoard', move: Tuple[int, int, int]) -> float:
        x, y, z = move
        flipped_count = len(board.flip_stones(x, y, z, 'B'))
        board.set_cell(x, y, z, 'B')

        counts = board.count_pieces()
        score = (counts['B'] - counts['W']) + self.weight_flip * flipped_count

        board.set_cell(x, y, z, '.')
        return score

    def play(self, board) -> Optional[Tuple[int, int, int]]:
        valid = self.get_valid_moves(board)
        if not valid:
            return None
        result = self.search(board, is_maximizing=True, depth=2)
        return result


class RandomAI:
    """ランダム AI（初心者向け）"""

    def __init__(self, name: str = "RandomAI"):
        self.name = name

    def play(self, board) -> Optional[Tuple[int, int, int]]:
        valid = board.get_valid_moves_for_color('B') if board.get_cell(0, 0, 0) == 'B' else \
                board.get_valid_moves_for_color('W')

        if not valid:
            return None

        # ランダムに手を選ぶ
        random.shuffle(valid)
        return valid[0]
