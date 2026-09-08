#include "board.hpp"
#include <iostream>
#include <algorithm>

namespace cubo {

// =============================================================================
// CubeBoard — 8×8x3 の立方体盤面管理
// 初期配置：中心部 (3,3)-(4,4) に黒・白を隣接させる
// =============================================================================

void CubeBoard::reset() {
    for (int x = 0; x < BOARD_SIZE; ++x)
        grid_[x] = std::array<PieceColor, BOARD_SIZE>();   // 空の array を代入
}

void CubeBoard::initialize() {
    reset();
    // 初期配置：中心部 (3,3)-(4,4) に黒・白を隣接させる（Q10: 簡易 AI）
    grid_[3][3] = PieceColor::BLACK;
    grid_[4][3] = PieceColor::WHITE;   // 黒の右隣に白
    grid_[3][4] = PieceColor::WHITE;    // 黒の下隣に白
    grid_[4][4] = PieceColor::BLACK;    // 白の右下隣に黒

    // デバッグ用：盤面描画
    std::cout << "Initial board configuration:\n";
}

PieceColor CubeBoard::get_cell(int x, int y) const {
    if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE)
        return PieceColor::EMPTY;
    // z は無視（2D 盤面として扱う）
    return grid_[x][y];
}

bool CubeBoard::set_cell(int x, int y, PieceColor color) {
    if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE)
        return false;
    // z は無視（2D 盤面として扱う）
    grid_[x][y] = color;
    return true;
}

// =============================================================================
// 挟み込み判定と石の反転（4 方向：±x, ±y、z は無視）
// =============================================================================

std::vector<std::tuple<int,int>> CubeBoard::flip_stones(
    int x, int y, PieceColor player_color) {

    std::vector<std::tuple<int,int>> flipped;

    // 4 方向の定義（±x, ±y）— z は無視
    const std::array<std::tuple<int,int>, 4> directions = {{
        {1, 0},   // +x
        {-1, 0},  // -x
        {0, 1},   // +y
        {0, -1}   // -y
    }};

    for (const auto& dir : directions) {
        int dx = std::get<0>(dir);
        int dy = std::get<1>(dir);

        int rx = x + dx, ry = y + dy;
        PieceColor opponent = (player_color == PieceColor::BLACK) ? PieceColor::WHITE : PieceColor::BLACK;

        // 相手の石の連続を確認（サンドウィッチの上側）
        while (rx >= 0 && rx < BOARD_SIZE && ry >= 0 && ry < BOARD_SIZE
               && grid_[rx][ry] == opponent) {
            rx += dx;
            ry += dy;
        }

        // 自分の石に達したか確認（サンドウィッチ成立）
        if (rx >= 0 && rx < BOARD_SIZE && ry >= 0 && ry < BOARD_SIZE
                && grid_[rx][ry] == player_color) {

            // 挟み込んだ相手の石を順次反転
            int cx = x + dx, cy = y + dy;
            while (true) {
                PieceColor piece = grid_[cx][cy];
                if (piece == player_color || piece == PieceColor::EMPTY) break;
                flipped.push_back({static_cast<int>(cx), static_cast<int>(cy)});
                grid_[cx][cy] = player_color;
                cx += dx;
                cy += dy;
            }
        }
    }

    return flipped;
}

std::vector<std::tuple<int,int>> CubeBoard::get_valid_moves_for_color(PieceColor color) {
    std::vector<std::tuple<int,int>> valid;
    for (int x = 0; x < BOARD_SIZE; ++x) {
        for (int y = 0; y < BOARD_SIZE; ++y) {
            if (grid_[x][y] == PieceColor::EMPTY) {
                auto flipped = flip_stones(x, y, color);
                if (!flipped.empty()) {
                    valid.push_back({static_cast<int>(x), static_cast<int>(y)});
                }
            }
        }
    }
    return valid;
}

std::vector<std::tuple<int,int>> CubeBoard::get_all_valid_moves() {
    std::vector<std::tuple<int,int>> valid;
    for (int x = 0; x < BOARD_SIZE; ++x) {
        for (int y = 0; y < BOARD_SIZE; ++y) {
            if (grid_[x][y] == PieceColor::EMPTY) {
                auto flipped_black = flip_stones(x, y, PieceColor::BLACK);
                auto flipped_white = flip_stones(x, y, PieceColor::WHITE);
                if (!flipped_black.empty() || !flipped_white.empty()) {
                    valid.push_back({static_cast<int>(x), static_cast<int>(y)});
                }
            }
        }
    }
    return valid;
}

std::pair<int,int> CubeBoard::count_pieces() const {
    int black = 0, white = 0;
    for (int x = 0; x < BOARD_SIZE; ++x) {
        for (int y = 0; y < BOARD_SIZE; ++y) {
            if (grid_[x][y] == PieceColor::BLACK) black++;
            else if (grid_[x][y] == PieceColor::WHITE) white++;
        }
    }
    return std::make_pair(black, white);
}

bool CubeBoard::is_full() const {
    for (int x = 0; x < BOARD_SIZE; ++x) {
        if (grid_[x][0] == PieceColor::EMPTY || grid_[x][BOARD_SIZE-1] == PieceColor::EMPTY)
            return false;
    }
    return true;
}

// =============================================================================
// テキスト描画（デバッグ用）
// =============================================================================

std::string CubeBoard::render() const {
    std::ostringstream oss;
    // ヘッダー
    oss << "\n╔══════╗\n║ BOARD ║\n╚══════╝\n\n";

    // 列ラベル（x 軸）
    for (int x = 0; x < BOARD_SIZE; ++x) {
        char label = 'A' + (x % 8);
        if (label > '9') label -= 7;
        oss << "   " << label << " \n";
    }

    // 行ラベル（y 軸）と盤面表示（上→下順、つまり y が大きい方から小さい方へ）
    for (int y = BOARD_SIZE - 1; y >= 0; --y) {
        oss << "   ";
        for (int x = 0; x < BOARD_SIZE; ++x) {
            PieceColor color = grid_[x][y];

            if (color == PieceColor::EMPTY) {
                oss << " · ";
            } else if (color == PieceColor::BLACK) {
                oss << " ⚫";
            } else if (color == PieceColor::WHITE) {
                oss << " ⚪";
            }

            // 有効手の表示（簡易：+ マーク）
            auto valid_moves = get_all_valid_moves();
            bool is_valid_move = false;
            for (const auto& mv : valid_moves) {
                if (std::get<0>(mv) == x && std::get<1>(mv) == y) {
                    is_valid_move = true;
                    break;
                }
            }
            if (is_valid_move) oss << " +";

            // 列ラベル
            char cx_label = 'A' + (x % 8);
            if (cx_label > '9') cx_label -= 7;
            oss << "\n      " << cx_label;
        }
    }

    return oss.str();
}

void CubeBoard::print() const {
    std::cout << render() << std::endl;
}

// =============================================================================
// オペレータオーバーロード（std::ostream への挿入）
// =============================================================================

namespace cubo {

std::ostream& operator<<(std::ostream& os, PieceColor color) {
    switch (color) {
        case PieceColor::EMPTY:  os << "·"; break;
        case PieceColor::BLACK:  os << '⚫'; break;
        case PieceColor::WHITE:  os << '⚪'; break;
        default:                 os << '?'; break;
    }
    return os;
}

} // namespace cubo

} // namespace cubo
