// Cube Othello — Board Module (C++17 / Terminal Fallback Mode)
#pragma once

#include <array>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <limits>

namespace cubo {

constexpr int BOARD_SIZE = 8;
constexpr char EMPTY_CHAR = '.';   // ASCII dot
constexpr char BLACK_CHAR = 'B';    // black stone (ASCII B)
constexpr char WHITE_CHAR = 'W';    // white stone (ASCII W)

// unscoped enum — Windows MinGW の互換性のため scoped enum を使用しない
enum PieceColor { EMPTY = 0, BLACK = 1, WHITE = 2 };


// =============================================================================
// CubeBoard — 8x8 の平面盤面（Q9: 平面表示方式）
// =============================================================================

class CubeBoard {
public:
    using BoardGrid = std::array<std::array<PieceColor, BOARD_SIZE>, BOARD_SIZE>;

private:
    BoardGrid grid_;
    PieceColor turn_player_ = BLACK;   // 現在のターン（黒が先攻）

public:
    CubeBoard() { for (int x = 0; x < BOARD_SIZE; ++x) grid_[x].fill(PieceColor::EMPTY); }
    ~CubeBoard() = default;

    void reset();
    void initialize();   // 初期配置：(3,3)-(4,4) に黒・白を隣接配置（Q10: 簡易 AI デバッグ用）

    PieceColor get_cell(int x, int y) const;
    bool set_cell(int x, int y, PieceColor color);   // 非 const — 盤面修改
    bool is_full() const;
    std::pair<int,int> count_pieces() const;          // ★宣言追加

    void print() const;
    std::string render() const;

    // public API: ターン管理、盤面操作（main.cpp や GameEngine でアクセスするため）
    PieceColor get_turn_player() const { return turn_player_; }
    PieceColor set_turn(PieceColor c) { turn_player_ = c; return turn_player_; }

    std::vector<std::pair<int,int>> get_valid_moves_for_color(PieceColor color) const;   // ★宣言追加
    std::vector<std::pair<int,int>> get_all_valid_moves() const;                        // ★宣言追加

private:
};


// =============================================================================
// CubeBoard 実装 — 全て inline で定義（cpp ファイル不要）
// =============================================================================

inline void CubeBoard::reset() {
    for (int x = 0; x < BOARD_SIZE; ++x) grid_[x].fill(PieceColor::EMPTY);
}

inline void CubeBoard::initialize() {
    reset();
    constexpr int black_x = 3, white_x = 4;
    for (int y = 3; y <= 4; ++y) {
        set_cell(black_x, y, BLACK);
        set_cell(white_x, y, WHITE);
    }
}

inline PieceColor CubeBoard::get_cell(int x, int y) const {
    if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE) return EMPTY;
    return grid_[x][y];   // z は無視 — Q9: 平面表示方式
}

inline bool CubeBoard::set_cell(int x, int y, PieceColor color) {
    if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE) return false;
    grid_[x][y] = color;   // z は無視（平面表示方式）
    return true;
}

inline bool CubeBoard::is_full() const {
    for (int x = 0; x < BOARD_SIZE; ++x) {
        if (grid_[x][0] == EMPTY || grid_[x][BOARD_SIZE-1] == EMPTY) return false;
    }
    return true;
}

inline std::pair<int,int> CubeBoard::count_pieces() const {
    int black = 0, white = 0;
    for (int x = 0; x < BOARD_SIZE; ++x) {
        for (int y = 0; y < BOARD_SIZE; ++y) {
            if (grid_[x][y] == BLACK) black++;
            else if (grid_[x][y] == WHITE) white++;
        }
    }
    return std::make_pair(black, white);
}

// テキスト表示：上→下順（y=7 が上段、y=0 が下段）、左→右順（A..H）
inline std::string CubeBoard::render() const {
    std::ostringstream oss;
    oss << "\n╔══════╗\n║ BOARD ║\n╚══════╝\n\n";

    for (int y = BOARD_SIZE - 1; y >= 0; --y) {
        oss << "   ";
        for (int x = 0; x < BOARD_SIZE; ++x) {
            PieceColor color = grid_[x][y];
            if (color == EMPTY) oss << " · ";
            else if (color == BLACK) oss << BLACK_CHAR;
            else if (color == WHITE) oss << WHITE_CHAR;

            oss << "\n      " << static_cast<char>('A' + x);
        }
    }
    return oss.str();
}

inline void CubeBoard::print() const { std::cout << render() << std::endl; }


// =============================================================================
// SimpleAI — Q10: 極めてシンプルなミニマックス AI（石数評価のみ、depth=2）
// =============================================================================

class SimpleAI {
public:
    enum class Action { NONE, PLACE_BLACK, PLACE_WHITE };

private:
    int depth_;
    PieceColor color_;   // ★ BLACK または WHITE — 色を固定して使用する
    bool is_maximizing_ = false;   // ★ 非 const メンバ変数（minimax の再帰で変更）

public:
    SimpleAI(int depth = 2, PieceColor color = WHITE)
        : depth_(depth), color_(color), is_maximizing_((color == BLACK)) {}

    std::pair<int,int> search(CubeBoard& board);   // ★ non-const ref — set_cell を呼べるように

private:
    int evaluate(const CubeBoard& board) const;   // ★ 評価のみ（const）

    // ★ minimax は CubeBoard の修改を行うため、non-const メソッドとして定義
    std::pair<std::pair<int,int>, int> minimax(CubeBoard& board, int depth_left, bool is_maximizing);
};


// =============================================================================
// SimpleAI 実装 — inline で定義（一度だけ、重複回避のため）
// =============================================================================

inline int SimpleAI::evaluate(const CubeBoard& board) const {
    auto [black_count, white_count] = count_pieces(board);
    int score = (color_ == BLACK) ? (black_count - white_count) : (white_count - black_count);
    return score;
}

// ★ minimax は non-const メソッド → set_cell を呼ぶため必要
inline std::pair<std::pair<int,int>, int> SimpleAI::minimax(CubeBoard& board, int depth_left, bool is_maximizing) {
    if (depth_left == 0 || board.is_full()) {
        auto moves = get_valid_moves_for_color(board, color_);
        if (!moves.empty()) return {{-1,-1}, evaluate(board)};

        PieceColor opp_color = (color_ == BLACK) ? WHITE : BLACK;
        auto opp_moves = get_valid_moves_for_color(board, opp_color);
        if (!opp_moves.empty()) {
            // 相手の手があるが、自分の評価関数で推定（簡易）
            return {{-1,-1}, evaluate(board)};
        }

        return {{-1,-1}, evaluate(board)};   // ドローまたはゲームオーバー
    }

    PieceColor my_color = is_maximizing ? BLACK : WHITE;
    auto valid_moves = get_valid_moves_for_color(board, my_color);

    if (valid_moves.empty()) {
        return {{-1,-1}, evaluate(board)};   // 自分の手がない → 不利な局面
    }

    int best_value = is_maximizing ? -std::numeric_limits<int>::max() : std::numeric_limits<int>::min();
    bool found_best = false;
    std::pair<int,int> best_move = {-1, -1};

    for (const auto& move : valid_moves) {
        int x = static_cast<int>(move.first);
        int y = static_cast<int>(move.second);

        // 仮の配置（盤面変更）
        board.set_cell(x, y, my_color);   // ★ set_cell は non-const なので OK

        auto result = minimax(board, depth_left - 1, !is_maximizing);   // ★ non-const ref で再帰呼び出し

        board.reset();   // ★ reset() を呼ぶ — non-const メソッドだが OK（board は非 const ref）

        if (!found_best) {
            best_value = result.second;
            best_move = move.first;
            found_best = true;
        } else if ((is_maximizing && result.second > best_value) ||
                   (!is_maximizing && result.second < best_value)) {
            best_value = result.second;
            best_move = move.first;
        }
    }

    return std::make_pair(best_move, evaluate(board));
}

inline std::pair<int,int> SimpleAI::search(CubeBoard& board) {   // ★ non-const ref で呼び出し元が移動できる
    if (board.is_full()) return {-1, -1};
    auto result = minimax(board, depth_, true);   // MAX（黒）から開始
    return result.first;                           // 最善手の座標
}


// =============================================================================
// CubeBoard: flip_stones と get_valid_moves_for_color — extern C ラッパー
// =============================================================================

extern "C" int flip_stones(int x, int y, PieceColor player_color);

inline std::vector<std::pair<int,int>> CubeBoard::get_valid_moves_for_color(PieceColor color) const {
    std::vector<std::pair<int,int>> valid;
    for (int i = 0; i < BOARD_SIZE; ++i) {
        for (int j = 0; j < BOARD_SIZE; ++j) {
            if (grid_[i][j] == EMPTY) {
                auto result = flip_stones(i, j, color);
                if (result > 0) {
                    valid.push_back({static_cast<int>(i), static_cast<int>(j)});
                }
            }
        }
    }
    return valid;
}

inline std::vector<std::pair<int,int>> CubeBoard::get_all_valid_moves() const {
    auto black = get_valid_moves_for_color(BLACK);
    auto white = get_valid_moves_for_color(WHITE);
    if (!black.empty()) return black;   // 黒の回合 → 優先
    if (!white.empty()) return white;
    return {};
}


// =============================================================================
// extern C ラッパー関数の実装（board.hpp 内で定義 — CubeBoard::grid_ に直接アクセス）
// =============================================================================

extern "C" int flip_stones(int x, int y, PieceColor player_color) {
    const PieceColor opponent = (player_color == BLACK) ? WHITE : BLACK;
    std::vector<std::pair<int,int>> flipped;

    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            if (dx == 0 && dy == 0) continue;   // 自身を飛ばない

            int rx = x + dx, ry = y + dy;

            if (rx < 0 || rx >= BOARD_SIZE || ry < 0 || ry >= BOARD_SIZE) continue;

            PieceColor piece = grid_[rx][ry];
            if (piece == opponent) {
                // 反転チェック（8 方向）
                for (int kx = -1; kx <= 1; ++kx) {
                    for (int ky = -1; ky <= 1; ++ky) {
                        if (kx == dx && ky == dy) continue;

                        int tx = rx + kx, ty = ry + ky;
                        if (tx < 0 || tx >= BOARD_SIZE || ty < 0 || ty >= BOARD_SIZE) break;

                        PieceColor target_piece = grid_[tx][ty];
                        if (target_piece == player_color) {
                            // 同色が見つかった → 反転成功（この方向）
                            for (int cx = rx; cx <= tx; ++cx) {
                                for (int cy = ry; cy <= ty; ++cy) {
                                    grid_[cx][cy] = player_color;
                                    flipped.push_back({static_cast<int>(cx), static_cast<int>(cy)});
                                }
                            }
                            return static_cast<int>(flipped.size());
                        } else if (target_piece != EMPTY && target_piece != opponent) {
                            // 中間に別の色の石がある → この方向は反転不可能
                            break;
                        }
                    }
                }
            }
        }
    }

    return static_cast<int>(flipped.size());
}


} // namespace cubo
