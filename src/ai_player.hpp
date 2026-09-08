// Cube Othello — AI Player (Simple Minimax)
#include <vector>
#include <tuple>
#include <algorithm>
#include <iostream>
#include "board.hpp"

namespace cubo {

class SimpleAI {
public:
    using BoardGrid = std::array<std::array<PieceColor, BOARD_SIZE>, BOARD_SIZE>;

private:
    int depth_ = 2;           // 探索深さ（0=貪欲、1-3=ミニマックス）
    PieceColor color_;        // AI の色
    bool is_black_start_;     // black-on-white を遵守するか

public:
    SimpleAI(int depth = 2, PieceColor color = PieceColor::WHITE)
        : depth_(depth), color_(color) {}

    // メイン検索関数
    std::pair<std::string, std::vector<std::tuple<int,int>>> search(
        const CubeBoard& board, bool black_start) {

        if (black_start && color_ == PieceColor::WHITE) {
            return {"PASS", {}};   // 黒が先攻 → 白はパス（簡易）
        } else if (!black_start && color_ == PieceColor::BLACK) {
            auto valid_moves = board.get_valid_moves_for_color(PieceColor::BLACK);
            return {"PLACE", valid_moves};
        }

        // white turn
        std::vector<std::tuple<int,int>> valid_moves;
        for (int x = 0; x < BOARD_SIZE; ++x) {
            for (int y = 0; y < BOARD_SIZE; ++y) {
                if (board.get_cell(x, y) == PieceColor::EMPTY) {
                    auto flipped = board.flip_stones(x, y, PieceColor::WHITE);
                    if (!flipped.empty()) {
                        valid_moves.push_back({static_cast<int>(x), static_cast<int>(y)});
                    }
                }
            }
        }

        if (valid_moves.empty()) {
            return {"PASS", {}};   // 有効な手が空 → パス
        }

        std::string action = "PLACE";
        auto best_move = valid_moves[0];  // MVP: 最初の有効手を採用（簡易）

        // 深さ制限付きミニマックス（簡易版：評価値のみ考慮）
        if (depth_ > 0) {
            int best_score = std::numeric_limits<int>::lowest();
            for (const auto& move : valid_moves) {
                PieceColor opponent = (color_ == PieceColor::BLACK) ? PieceColor::WHITE : PieceColor::BLACK;

                // 仮置換と反転
                board.set_cell(std::get<0>(move), std::get<1>(move), color_);
                auto flipped = board.flip_stones(std::get<0>(move), std::get<1>(move), color_);
                board.reset();  // 元に戻す

                int score = evaluate_move(board, opponent);

                if (score > best_score) {
                    best_score = score;
                    best_move = move;
                }
            }
        }

        return {"PLACE", {best_move}};
    }

private:
    // 簡易評価関数：石数差のみ考慮
    int evaluate_move(const CubeBoard& board, PieceColor opponent) const {
        auto counts = board.count_pieces();
        int black_count = std::get<0>(counts);
        int white_count = std::get<1>(counts);

        // 相手の石数を最大化（評価関数）
        return (opponent == PieceColor::BLACK) ? (black_count - white_count) :
                   (white_count - black_count);
    }
};

} // namespace cubo
