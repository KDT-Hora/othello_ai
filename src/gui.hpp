// Cube Othello — GUI Module (C++ / DxLib) v2.0
#pragma once

#include <array>
#include <vector>
#include <string>
#include <cmath>
#include "board.hpp"
#include <windows.h>   // Win32 API for input handling

namespace cubo {

constexpr int SCREEN_W = 800;
constexpr int SCREEN_H = 600;
constexpr float SCALE = 2.5f;

// ============================================================================
class CubeGridRenderer {
public:
    using BoardGrid = std::array<std::array<PieceColor, BOARD_SIZE>, BOARD_SIZE>;
private:
    const BoardGrid& board_;
    bool dxlib_available_ = false;
    int screen_width_ = SCREEN_W;
    int screen_height_ = SCREEN_H;

public:
    enum class RenderMode { DXLIB, TERMINAL };
    RenderMode mode() const { return dxlib_available_ ? RenderMode::DXLIB : RenderMode::TERMINAL; }

    CubeGridRenderer(const BoardGrid& board)
        : board_(board), screen_width_(SCREEN_W), screen_height_(SCREEN_H) {}

    // DxLib 描画（メイン）
    void draw_dxlib() const;

    // ターミナルフォールバック描画
    std::string render_terminal() const;

private:
    static constexpr float PI_OVER_4 = 0.78539816f;

    D3DPoint project_point(int x, int y, int z) const {
        float angle = M_PI_F / 4.0f + (static_cast<float>(z) - 4.0f) * PI_OVER_4;
        return {
            static_cast<int>((x - z) * SCALE),
            static_cast<int>(-(y + z) * SCALE / 2.0f),
        };
    }

    void draw_wireframe() const;   // ワイヤーフレーム描画
    void draw_pieces() const;      // 石の描画（円形＋明度グラデーション）
    void draw_valid_move_markers() const;   // 「+」マーク表示
};

// ============================================================================
class GameGUI {
public:
private:
    CubeBoard board_;
    PieceColor turn_player_ = BLACK;
    bool game_over_ = false;
    char winner_ = '.';

public:
    GameGUI() : board_{} { board_.initialize(); }

    int main_loop();   // メインループ（DxLib 描画＋入力処理）
};

} // namespace cubo
