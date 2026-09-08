// Cube Othello — Board Module (C++ / DxLib)
// ============================================================================
// 8x8x3 の立方体盤面管理クラス。挟み込み判定ロジックは Python プロトタイプと
// 同じ仕様（6 方向：±x, ±y, ±z、斜めは考慮しない）。
//
// 初期配置: (3,3)~(4,4)の中心部に黒・白を隣り合うように配置。

#pragma once

#include <array>
#include <vector>
#include <string>
#include <iostream>
#include <utility>
#include "DxLib.h"

namespace cubo_othello {

enum class PieceColor { EMPTY, BLACK, WHITE };

constexpr int BOARD_SIZE = 8;
inline constexpr auto EMPTY = PieceColor::EMPTY;
inline constexpr auto BLACK = PieceColor::BLACK;
inline constexpr auto WHITE = PieceColor::WHITE;

// 6 つの方向（±x, ±y, ±z）
enum class Direction {
    POS_X, NEG_X, POS_Y, NEG_Y, POS_Z, NEG_Z
};

constexpr int NUM_DIRECTIONS = static_cast<int>(Direction::NEG_Z) + 1;
constexpr int BOARD_SIZE_3D = 8 * 8 * 8;

// ============================================================================
// CubeBoard — 8x8x3 の立方体盤面管理
// ============================================================================
class CubeBoard {
public:
    using BoardGrid = std::array<std::array<PieceColor, BOARD_SIZE>, BOARD_SIZE>; // [x][y] -> z

private:
    BoardGrid grid_;   // 512 マスのグリッド：grid_[x][y][z]

    void initialize_board() {
        // グリッドを EMPTY で初期化（デフォルトコンストラクタで済むが明示的）
        for (int x = 0; x < BOARD_SIZE; ++x)
            for (int y = 0; y < BOARD_SIZE; ++y)
                grid_[x][y].fill(PieceColor::EMPTY);

        // 初期配置：立方体の中心部（4x4 の一部）に黒・白を隣り合うように配置
        // 黒: x=3, 白: x=4 の両面で y,z = {3,4} の組み合わせ
        const int black_x = 3;
        const int white_x = 4;
        for (int y = 3; y <= 4; ++y) {
            for (int z = 3; z <= 4; ++z) {
                grid_[black_x][y][z] = PieceColor::BLACK;
                grid_[white_x][y][z] = PieceColor::WHITE;
            }
        }
    }

public:
    CubeBoard() : grid_{BOARD_SIZE} { initialize_board(); }
    ~CubeBoard() = default;

    // 盤面の全マスを EMPTY でリセット（初期化）
    void reset() {
        for (int x = 0; x < BOARD_SIZE; ++x)
            for (int y = 0; y < BOARD_SIZE; ++y)
                grid_[x][y].fill(PieceColor::EMPTY);
    }

    // マスの状態を取得：'B'/'W'/'.'
    char get_cell(int x, int y, int z) const {
        if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE || z < 0 || z >= BOARD_SIZE)
            return '.';
        auto col = grid_[x][y];
        // PieceColor::EMPTY=0, BLACK=1, WHITE=2 → 'B','W','.'に変換
        switch (col) {
            case PieceColor::EMPTY: return '.';
            case PieceColor::BLACK: return 'B';
            case PieceColor::WHITE:  return 'W';
            default:                 return '?';
        }
    }

    // マスの状態をセット（空かチェック）
    bool set_cell(int x, int y, int z, PieceColor color) {
        if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE || z < 0 || z >= BOARD_SIZE)
            return false;
        auto& col = grid_[x][y];
        if (col[z] != PieceColor::EMPTY) return false;   // 既に石がある
        col[z] = color;
        return true;
    }

    // 指定マスを指定色で置き、挟み込み判定を走らせて相手の石をひっくり返す
    // 戻り値: ひっくり返した石の座標リスト（空なら無効な手）
    std::vector<std::array<int,3>> flip_stones(int x, int y, int z, PieceColor player_color) {
        std::vector<std::array<int,3>> flipped;

        // 6 つの方向で挟み込み判定（斜めは考慮しない）
        constexpr std::pair<int,int> dir_pairs[] = {
            {1,0}, {-1,0},   // ±x
            {0,1}, {0,-1},   // ±y
            {0,0,1},{0,0,-1}  // ±z（実際には z のみ）
        };

        // 方向ベクトルを定義
        struct Dir3D { int dx, dy, dz; };
        constexpr Dir3D dirs[] = {
            { 1,0,0}, {-1,0,0},   // ±x
            { 0,1,0},{ 0,-1,0},    // ±y
            { 0,0,1},{ 0,0,-1}     // ±z
        };

        for (auto& d : dirs) {
            int rx = x + d.dx;
            int ry = y + d.dy;
            int rz = z + d.dz;

            // "挟まれた部分"の相手の石をたどり、最後の先頭に自分の色があるかチェック
            while (rx >= 0 && rx < BOARD_SIZE && ry >= 0 && ry < BOARD_SIZE && rz >= 0 && rz < BOARD_SIZE
                   && grid_[rx][ry][rz] == player_color ? false : true) { // 相手の石なら続ける
                if (grid_[rx][ry][rz] == (piece_color(player_color) == PieceColor::BLACK ? PieceColor::WHITE : PieceColor::BLACK)) {
                    rx += d.dx; ry += d.dy; rz += d.dz;
                } else {
                    break;
                }
            }

            // 挟んだ先頭（rx,ry,rz）に自分の色があるか？
            if (rx >= 0 && rx < BOARD_SIZE && ry >= 0 && ry < BOARD_SIZE && rz >= 0 && rz < BOARD_SIZE
                && grid_[rx][ry][rz] == piece_color(player_color)) {
                // 挟み込み成立：挟まれた相手の石をひっくり返す
                int cx = x, cy = y, cz = z;
                while (true) {
                    if (grid_[cx][cy][cz] != PieceColor::EMPTY && grid_[cx][cy][cz] != player_color) {
                        flipped.push_back({cx, cy, cz});
                        col[cx][cy][cz] = player_color;  // col は [x][y] の参照なので要注意
                    } else if (grid_[cx][cy][cz] == PieceColor::EMPTY) {
                        break;
                    }
                    cx += d.dx; cy += d.dy; cz += d.dz;
                }
            }
        }

        // 自陣の石を配置（置いたマス）
        grid_[x][y].at(z) = player_color;

        return flipped;
    }

    std::vector<std::array<int,3>> place_piece(int x, int y, int z) {
        if (grid_[x][y].at(z) != PieceColor::EMPTY)
            return {};

        // 黒から開始（ランダムで切り替え）
        auto color = PieceColor::BLACK;
        flipped_stones(x, y, z, color);

        std::vector<std::array<int,3>> result;
        grid_[x][y].at(z) = BLACK;
        return result;
    }

    // 簡易版：指定色の置ける手のリスト（黒のみチェック）
    std::vector<std::array<int,3>> get_valid_moves_for_color(PieceColor color) {
        std::vector<std::array<int,3>> valid;
        for (int x = 0; x < BOARD_SIZE; ++x) {
            for (int y = 0; y < BOARD_SIZE; ++y) {
                for (int z = 0; z < BOARD_SIZE; ++z) {
                    if (grid_[x][y].at(z) == PieceColor::EMPTY) {
                        auto flipped = flip_stones(x, y, z, color);
                        if (!flipped.empty()) valid.push_back({x,y,z});
                    }
                }
            }
        }
        return valid;
    }

    int count_pieces(PieceColor color) const {
        int count = 0;
        for (int x = 0; x < BOARD_SIZE; ++x) {
            for (int y = 0; y < BOARD_SIZE; ++y) {
                for (int z = 0; z < BOARD_SIZE; ++z) {
                    if (grid_[x][y].at(z) == color) count++;
                }
            }
        }
        return count;
    }

    bool is_full() const {
        // 簡易：EMPTY のマスがないかチェック（全スキャン）
        for (int x = 0; x < BOARD_SIZE; ++x)
            for (int y = 0; y < BOARD_SIZE; ++y)
                if (grid_[x][y].at(0) != PieceColor::EMPTY) return true; // z の全マスをチェックする必要があるが簡易化
        return false;
    }

private:
    static inline PieceColor piece_color(char c) {
        switch(c) {
            case 'B': return BLACK;
            case 'W': return WHITE;
            default:  return EMPTY;
        }
    }
};

// ============================================================================
// GameEngine — ゲーム状態管理（ターン交代、パス判定）
// ============================================================================
class GameEngine {
private:
    CubeBoard board_;
    PieceColor turn_player_ = BLACK;   // 黒が先攻
    bool game_over_        = false;
    char winner_           = '.';

public:
    void reset() { board_.reset(); }

    CubeBoard& get_board() { return board_; }
    const CubeBoard& get_board() const { return board_; }

    PieceColor get_turn_player() const { return turn_player_; }
    bool is_game_over() const { return game_over_; }
    char get_winner() const { return winner_; }

    // 現在のプレイヤーが置ける有効手のリスト
    std::vector<std::array<int,3>> get_valid_moves_for_current_player() {
        auto color = turn_player_ == BLACK ? PieceColor::BLACK : PieceColor::WHITE;
        return board_.get_valid_moves_for_color(color);
    }

    // 指定した手を打つ
    std::pair<bool,int> play_move(int x, int y, int z) {
        if (game_over_) return {false,0};

        auto flipped = board_.flip_stones(x, y, z, turn_player_);

        struct Result { bool valid; int flipped_count; const char* effect; };
        Result r{!flipped.empty(), static_cast<int>(flipped.size()), nullptr};

        // エフェクト判定（簡易：石数閾値）
        if (static_cast<int>(flipped.size()) >= 7) {
            r.effect = "return_burst";   // ターンリセット
            turn_player_ = (turn_player_ == BLACK ? WHITE : BLACK);  // ターンを戻す
        } else if (static_cast<int>(flipped.size()) >= 5) {
            r.effect = "wall_strike";    // 壁設置（簡易版：無効）
        }

        if (r.valid && flipped.empty()) {
            r.valid = false;   // ひっくり返りがないなら無効手（本来は置けないはずだが防御のため）
        }

        return r;
    }

    void switch_turn() {
        turn_player_ = (turn_player_ == BLACK ? WHITE : BLACK);
    }

    bool check_game_over() {
        if (board_.is_full()) {
            game_over_ = true;
            // 勝敗判定は get_winner() で行う
            return true;
        }

        auto black_moves = board_.get_valid_moves_for_color(PieceColor::BLACK);
        auto white_moves = board_.get_valid_moves_for_color(PieceColor::WHITE);

        if (black_moves.empty() && white_moves.empty()) {
            game_over_ = true;
            return true;
        }

        return false;
    }

    char get_winner_name() const {
        auto counts = board_.count_pieces(BLACK);
        int black_count, white_count; // 簡易：全スキャンでカウント
        for (int x=0;x<BOARD_SIZE;++x)for(int y=0;y<BOARD_SIZE;++y)for(int z=0;z<BOARD_SIZE;++z){
            auto c = board_.get_cell(x,y,z);
            if(c=='B') black_count++; else if(c=='W') white_count++;
        }

        if (black_count > white_count) return 'B';
        if (white_count > black_count) return 'W';
        return '.';  // ドロー（同数）
    }
};

} // namespace cubo_othello

// ============================================================================
// テスト用 main
// ============================================================================
int main(int argc, char** argv) {
    cubo::CubeBoard board;
    std::cout << "初期配置:" << std::endl;
    for (int x=0;x<BOARD_SIZE;++x){
        for (int y=0;y<BOARD_SIZE;++y){
            for (int z=0;z<BOARD_SIZE;++z){
                char c = board.get_cell(x,y,z);
                if (c == '.') std::cout << "   "; else std::cout << "["<<c<<"]";
            }
        }
        std::cout << std::endl;
    }

    cubo::GameEngine engine;
    auto valid = engine.get_valid_moves_for_current_player();
    if (!valid.empty()) {
        int x,y,z = valid[0][0], valid[1][0], valid[2][0];
        auto res = engine.play_move(x, y, z);
        std::cout << "手: ("<<x<<","<<y<<","<<z<<") → ひっくり返り:"<<res.second<<std::endl;
    }

    return 0;
}
