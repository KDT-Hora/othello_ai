#pragma once

#include "ai_player.hpp"

namespace cubo {

// ============================================================================
// SimpleAI Implementation — Minimax AI with stone-count evaluation
// ============================================================================

std::vector<std::tuple<int, int, int>> get_valid_moves(
    const CubeBoard& board, PieceColor color) {
    return board.get_valid_moves_for_color(color);
}

int evaluate_move(const CubeBoard& board, int x, int y, int z,
                  PieceColor color, int depth) {

    // 簡易評価：石数のみ（深さ制限付き）
    auto counts = board.count_pieces();
    int score = (color == PieceColor::BLACK) ? counts.first : counts.second;

    return score;
}

// メイン検索関数の実装
std::pair<std::string, std::vector<std::tuple<int, int, int>>> SimpleAI::search(
    const CubeBoard& board, bool black_start) {

    if (black_start && color_ == PieceColor::WHITE) {
        return {'PASS', {}};   // 黒が先攻 → 白はパス
    } else if (!black_start && color_ == PieceColor::BLACK) {
        auto valid = get_valid_moves(board, PieceColor::BLACK);
        return {'PLACE', valid};
    }

    auto valid_moves = board.get_valid_moves_for_color(color_);
    if (valid_moves.empty()) {
        return {'PASS', {}};
    }

    // 貪欲な選択（石数のみ）
    int best_score = -9999;
    std::vector<std::tuple<int,int,int>> best_moves;

    for (const auto& move : valid_moves) {
        auto flipped = board.flip_stones(std::get<0>(move), std::get<1>(move), 
                                          std::get<2>(move), color_);
        int score = static_cast<int>(flipped.size());   // 反転数で評価

        if (score > best_score) {
            best_score = score;
            best_moves.clear();
            best_moves.push_back(move);
        } else if (score == best_score && !best_moves.empty()) {
            best_moves.push_back(move);   // スコア同点なら追加（ランダム選択用）
        }
    }

    return {'PLACE', best_moves};
}

} // namespace cubo
