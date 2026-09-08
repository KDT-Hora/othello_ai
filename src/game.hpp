// Cube Othello — Game Engine (C++ / Terminal Fallback)
#pragma once

#include <array>
#include <vector>
#include <string>
#include <tuple>
#include "board.hpp"

namespace cubo {

constexpr int BOARD_SIZE = 8;
constexpr char EMPTY = '.';
constexpr char BLACK = '⚫';
constexpr char WHITE = '⚪';

enum class PieceColor : char { EMPTY, BLACK, WHITE };

// ============================================================================
class GameEngine {
public:
    using BoardGrid = std::array<std::array<PieceColor, BOARD_SIZE>, BOARD_SIZE>;

private:
    CubeBoard board_;
    PieceColor turn_player_ = PieceColor::BLACK;   // 黒が先攻（black-on-white）
    bool game_over_ = false;
    char winner_ = '.';

public:
    GameEngine() : board_{} {
        board_.initialize();   // 初期配置：中心部で黒・白を隣り合うように配置
    }

    // ゲームループのメイン処理（ターミナルフォールバックモード）
    int run() {
        std::cout << "╔══════════════════════════════════════════════╗\n";
        std::cout << "║           CUBE OTHELLO — Terminal Mode       ║\n";
        std::cout << "╚══════════════════════════════════════════════╝\n\n";

        int move_count = 0;

        while (!game_over_) {
            // 盤面描画
            render_board();

            // ゲーム終了チェック
            if (is_game_over()) {
                render_game_over();
                break;
            }

            // ターン交代表示
            std::string turn_str = (turn_player_ == PieceColor::BLACK) ? "⚫ BLACK" : "⚪ WHITE";
            std::cout << "\n━━━ Turn: [" << turn_str << "] ━━━\n";

            // 有効な手の取得
            auto valid_moves = board_.get_all_valid_moves();

            if (valid_moves.empty()) {
                // 両プレイヤーとも置けない → パスまたはドロー判定
                std::cout << "No valid moves available — passing turn...\n\n";
                // ターン交代（簡易：黒がパスし、白のターンへ）
                turn_player_ = (turn_player_ == PieceColor::BLACK) ? PieceColor::WHITE : PieceColor::BLACK;
            } else {
                std::cout << "Valid moves: ";
                for (const auto& mv : valid_moves) {
                    std::cout << "(" << std::get<0>(mv) << ","
                              << std::get<1>(mv) << ","
                              << std::get<2>(mv) << ") ";
                }
                std::cout << "\n";

                // 簡易：ランダムな有効手を選択（AI モードならミニマックスを使用）
                auto& move = valid_moves[0];   // 簡易化：最初の有効手を選択
                board_.set_cell(std::get<0>(move), std::get<1>(move), std::get<2>(move), turn_player_);

                // 挟み込みによる石の反転
                auto flipped = board_.flip_stones(std::get<0>(move),
                                                   std::get<1>(move),
                                                   std::get<2>(move),
                                                   turn_player_);

                if (!flipped.empty()) {
                    int count = static_cast<int>(flipped.size());
                    std::cout << "Flipped " << count << " stone(s)!\n";
                } else {
                    std::cout << "(No sandwich — piece placed without flip)\n";
                }

                move_count++;
            }

            // 簡易：ターン交代（パス判定は後で）
            turn_player_ = (turn_player_ == PieceColor::BLACK) ? PieceColor::WHITE : PieceColor::BLACK;
        }

        render_final_score();
        return 0;
    }

private:
    void render_board() {
        // ヘッダー
        std::cout << "\n╔═══════════════════════════╗\n";
        std::cout << "║         CUBE BOARD (z=3)   ║\n";
        std::cout << "╚═══════════════════════════╝\n";

        // 列ラベル（x 軸）
        std::cout << "   ";
        for (int x = 0; x < BOARD_SIZE; ++x) {
            char label = 'A' + (x % 8);   // A-H のラベル
            if (label > '9') label -= 7;   // A→1, B→2, ..., H→8
            std::cout << " " << label << " ";
        }
        std::cout << "\n";

        // 各行の描画
        for (int y = BOARD_SIZE - 1; y >= 0; --y) {   // 逆順で表示（上→下）
            std::cout << "   ";
            for (int x = 0; x < BOARD_SIZE; ++x) {
                PieceColor color = board_.get_cell(x, y, 3);   // z=3（中心面）を表示

                if (color == PieceColor::EMPTY) {
                    std::cout << " · ";
                } else if (color == PieceColor::BLACK) {
                    std::cout << " ⚫ ";
                } else if (color == PieceColor::WHITE) {
                    std::cout << " ⚪ ";
                }

                // 有効手の表示（+ マーク）
                auto valid_moves = board_.get_all_valid_moves();
                bool is_valid_move = false;
                for (const auto& mv : valid_moves) {
                    if (std::get<0>(mv) == x && std::get<1>(mv) == y) {
                        is_valid_move = true;
                        break;
                    }
                }
                if (is_valid_move) {
                    std::cout << " + ";   // 有効手の表示
                } else {
                    std::cout << "   ";
                }
            }
            std::cout << '\n';

            // 行ラベル（y 軸：8→7→...→0）
            int label = BOARD_SIZE - 1 - y;
            if (label >= 10) {
                std::cout << "  " << label << "\n";
            } else {
                std::cout << "   " << label << "\n";
            }
        }

        // z=4 の層も表示（簡易化：z=3 と同じ）
        std::cout << "\n╔═══════════════════════════╗\n";
        std::cout << "║         CUBE BOARD (z=4)   ║\n";
        std::cout << "╚═══════════════════════════╝\n";

        // ヘッダー（列ラベル）
        std::cout << "   ";
        for (int x = 0; x < BOARD_SIZE; ++x) {
            char label = 'A' + (x % 8);
            if (label > '9') label -= 7;
            std::cout << " " << label << " ";
        }
        std::cout << "\n";

        // 各行の描画（z=4）
        for (int y = BOARD_SIZE - 1; y >= 0; --y) {
            std::cout << "   ";
            for (int x = 0; x < BOARD_SIZE; ++x) {
                PieceColor color = board_.get_cell(x, y, 4);

                if (color == PieceColor::EMPTY) {
                    std::cout << " · ";
                } else if (color == PieceColor::BLACK) {
                    std::cout << " ⚫ ";
                } else if (color == PieceColor::WHITE) {
                    std::cout << " ⚪ ";
                }

                auto valid_moves = board_.get_all_valid_moves();
                bool is_valid_move = false;
                for (const auto& mv : valid_moves) {
                    if (std::get<0>(mv) == x && std::get<1>(mv) == y) {
                        is_valid_move = true;
                        break;
                    }
                }
                if (is_valid_move) {
                    std::cout << " + ";
                } else {
                    std::cout << "   ";
                }
            }
            std::cout << '\n';

            int label = BOARD_SIZE - 1 - y;
            if (label >= 10) {
                std::cout << "  " << label << "\n";
            } else {
                std::cout << "   " << label << "\n";
            }
        }
    }

    bool is_game_over() const {
        // 盤面が埋まっているか確認
        bool all_filled = true;
        for (int x = 0; x < BOARD_SIZE; ++x) {
            for (int y = 0; y < BOARD_SIZE; ++y) {
                if (board_.get_cell(x, y, 3) == PieceColor::EMPTY ||
                    board_.get_cell(x, y, 4) == PieceColor::EMPTY) {
                    all_filled = false;
                    break;
                }
            }
        }

        // ドロー判定（両プレイヤーとも有効な手が置けない）
        bool black_has_moves = !board_.get_all_valid_moves().empty() ||
                               board_.get_cell(3, 3, 4) != PieceColor::EMPTY ||   // z=4 で黒がいる
                               board_.get_cell(4, 4, 0) != PieceColor::WHITE;    // z=0 で白がいる

        bool white_has_moves = !board_.get_all_valid_moves().empty() ||
                               board_.get_cell(3, 3, 4) != PieceColor::EMPTY ||   // z=4 で黒がいる
                               board_.get_cell(4, 4, 0) != PieceColor::WHITE;    // z=0 で白がいる

        if (!all_filled && !black_has_moves && !white_has_moves) {
            game_over_ = true;
            winner_ = 'D';   // ドロー
            return true;
        }

        return false;
    }

    void render_game_over() const {
        std::cout << "\n═══════════════ GAME OVER ═══════════════=\n";

        PieceColor last_turn = board_.get_cell(3, 4, 4) == PieceColor::BLACK ?
                              PieceColor::BLACK : (board_.get_cell(3, 4, 4) == PieceColor::WHITE ?
                                                   PieceColor::WHITE : PieceColor::EMPTY);

        if (winner_ == 'D') {
            std::cout << "Result: DRAW (no valid moves for both players)\n";
        } else if (board_.count_pieces().first > board_.count_pieces().second) {
            std::cout << "Result: BLACK WINS! ("
                      << board_.count_pieces().first << " vs "
                      << board_.count_pieces().second << " stones)\n";
        } else if (board_.count_pieces().second > board_.count_pieces().first) {
            std::cout << "Result: WHITE WINS! ("
                      << board_.count_pieces().first << " vs "
                      << board_.count_pieces().second << " stones)\n";
        }

        std::cout << "═══════════════ END ═══════════════════════\n";
    }

    void render_final_score() const {
        auto counts = board_.count_pieces();
        int black_count = counts.first;
        int white_count = counts.second;

        // 石数のカウント（簡易：盤面全スキャン）
        for (int x = 0; x < BOARD_SIZE; ++x) {
            for (int y = 0; y < BOARD_SIZE; ++y) {
                for (int z = 0; z < BOARD_SIZE; ++z) {
                    if (board_.get_cell(x, y, z) == PieceColor::BLACK) black_count++;
                    else if (board_.get_cell(x, y, z) == PieceColor::WHITE) white_count++;
                }
            }
        }

        std::cout << "\n━━━━━━━━━━━━━━━ FINAL SCORE ━━━━━━━━━━━━━━━━\n";
        std::cout << "   BLACK:  " << black_count << " stones\n";
        std::cout << "   WHITE:  " << white_count << " stones\n";
    }

public:
    const CubeBoard& get_board() const { return board_; }
};

} // namespace cubo
