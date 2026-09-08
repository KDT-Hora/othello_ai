// Cube Othello — GUI Module (C++ / DxLib) v2.0
// ============================================================================
// DxLib を用いた等角投影による 3D グリッド描画。
// ターミナルフォールバックモードも内蔵（DxLib が未リンクの場合）。

#pragma once

#include <array>
#include <vector>
#include <string>
#include <cmath>
#include "board.hpp"
#include <windows.h>  // Win32 API for sound (simplified)

namespace cubo {

constexpr int SCREEN_W = 800;
constexpr int SCREEN_H = 600;
constexpr float SCALE = 2.5f;
constexpr int CENTER_X = SCREEN_W / 2;
constexpr int CENTER_Y = SCREEN_H / 2;

// ============================================================================
struct D3DPoint {
    float x, y, z;   // 3D 座標（盤面座標）
};

// ============================================================================
class CubeGridRenderer {
public:
    using BoardGrid = std::array<std::array<PieceColor, BOARD_SIZE>, BOARD_SIZE>;

private:
    const BoardGrid& board_;
    bool dxlib_available_ = false;   // DxLib の利用可否
    int screen_width_ = SCREEN_W;
    int screen_height_ = SCREEN_H;

public:
    enum class RenderMode { DXLIB, TERMINAL };
    RenderMode mode() const { return dxlib_available_ ? RenderMode::DXLIB : RenderMode::TERMINAL; }

    CubeGridRenderer(const BoardGrid& board)
        : board_(board), screen_width_(SCREEN_W), screen_height_(SCREEN_H) {}

    // DxLib 描画（メイン）
    void draw_dxlib() const {
        if (!dxlib_available_) return;

        // 背景クリア
        DrawFilledRectangle(0, 0, screen_width_, screen_height_, RGB(15, 20, 35));

        // ワイヤーフレーム（立方体の外枠）
        draw_wireframe();

        // 各面の描画
        for (int face = 0; face < 6; ++face) {
            float angle = static_cast<float>(face) * M_PI_F / 3.0f + 0.25f * M_PI_F;
            draw_face(face, angle);
        }

        // 有効手の表示（+ マーク）
        for (const auto& move : get_all_valid_moves()) {
            int px = project_x(move[0]);
            int py = project_y(move[2]);   // z 軸を y 方向に投影
            DrawTextAt(px, py - 16, "+", RGB(50, 200, 50));
        }

        // UI パネル
        update_ui_panel();
    }

    // ターミナルフォールバック描画（絵文字＋ANSI コード）
    std::string render_terminal() const {
        if (dxlib_available_) return {};   // DxLib ありの場合は空（メインで使うのは draw_dxlib）

        std::string output;
        for (int z = BOARD_SIZE - 1; z >= 0; --z) {
            output += "[";
            for (int x = 0; x < BOARD_SIZE; ++x) {
                char ch = '.';
                switch (board_[x][3][z]) {   // z=3（中心面）を表示
                    case PieceColor::EMPTY: ch = '.'; break;
                    case PieceColor::BLACK: ch = '⚫'; break;
                    case PieceColor::WHITE:  ch = '⚪'; break;
                }
                output += ch;
            }
            output += "] (z=" + std::to_string(z) + ")\n";
        }
        return output;
    }

private:
    static constexpr float PI_OVER_4 = 0.78539816f;   // π/4

    D3DPoint project_point(int x, int y, int z) const {
        float angle = M_PI_F / 4.0f + (static_cast<float>(z) - 4.0f) * PI_OVER_4;
        return {
            static_cast<int>((x - z) * SCALE * 128.f / BOARD_SIZE),   // x: 等角投影（横方向）
            static_cast<int>((y + z) * SCALE * 64.f / BOARD_SIZE),    // y: 等角投影（縦方向、縮小）
        };
    }

    int project_x(int x, int z) const { return (x - z) * SCALE; }
    int project_y(int y, int z) const { return -(y + z) * SCALE / 2.0f; }

    void draw_wireframe() const {
        // 立方体の外枠（ワイヤーフレーム）
        constexpr float thickness = 1.5f;   // ワイヤーの太さ

        for (int x = 0; x <= BOARD_SIZE; ++x) {
            int p1_x = project_point(x, 0, BOARD_SIZE - 1).x + CENTER_X;
            int p1_y = project_point(x, 0, BOARD_SIZE - 1).y + CENTER_Y;
            int p2_x = project_point(x, 0, 0).x + CENTER_X;
            int p2_y = project_point(x, 0, 0).y + CENTER_Y;
            DrawLine(p1_x, p1_y, p2_x, p2_y);   // 前面の底辺
            DrawLine(p1_x, p1_y - SCALE * BOARD_SIZE / 2.0f, p2_x, p2_y - SCALE * BOARD_SIZE / 2.0f);
        }

        for (int z = 0; z <= BOARD_SIZE; ++z) {
            int p1_x = project_point(BOARD_SIZE, BOARD_SIZE, z).x + CENTER_X;
            int p1_y = project_point(BOARD_SIZE, BOARD_SIZE, z).y + CENTER_Y;
            int p2_x = project_point(0, BOARD_SIZE, z).x + CENTER_X;
            int p2_y = project_point(0, BOARD_SIZE, z).y + CENTER_Y;
            DrawLine(p1_x, p1_y, p2_x, p2_y);   // 右面の底辺
        }

        for (int y = 0; y <= BOARD_SIZE; ++y) {
            int p1_x = project_point(BOARD_SIZE, y, BOARD_SIZE - 1).x + CENTER_X;
            int p1_y = project_point(BOARD_SIZE, y, BOARD_SIZE - 1).y + CENTER_Y;
            int p2_x = project_point(0, y, BOARD_SIZE - 1).x + CENTER_X;
            int p2_y = project_point(0, y, BOARD_SIZE - 1).y + CENTER_Y;
            DrawLine(p1_x, p1_y, p2_x, p2_y);   // 右面の頂辺
        }

        // 前面のグリッド線（x 方向）
        for (int i = 0; i < BOARD_SIZE - 1; ++i) {
            int px_front = CENTER_X + project_point(i, 4, 0).x;   // y=4 を中心として
            int py_front = CENTER_Y - project_point(3, i, 0).y;   // x=3 を中心として
            int p1_x = px_front - SCALE * BOARD_SIZE / 2.0f + (i == 0 ? 0 : SCALE);
            int p1_y = py_front;
            int p2_x = px_front + SCALE * BOARD_SIZE / 2.0f - (i == BOARD_SIZE - 2 ? 0 : SCALE);
            int p2_y = py_front;
            DrawLine(p1_x, p1_y, p2_x, p2_y);
        }

        // 右面のグリッド線（z 方向）
        for (int i = 0; i < BOARD_SIZE - 1; ++i) {
            int px_right = CENTER_X + project_point(BOARD_SIZE, 4, i).x;
            int py_right = CENTER_Y - project_point(3, 4, i).y;
            int p1_x = px_right + SCALE * BOARD_SIZE / 2.0f;
            int p1_y = py_right + (i == 0 ? 0 : -SCALE);
            int p2_x = px_right + SCALE * BOARD_SIZE / 2.0f;
            int p2_y = py_right + (i == BOARD_SIZE - 2 ? 0 : -SCALE);
            DrawLine(p1_x, p1_y, p2_x, p2_y);
        }

        // 左面のグリッド線（x 方向、裏側）
        for (int i = 0; i < BOARD_SIZE - 1; ++i) {
            int px_left = CENTER_X + project_point(i, BOARD_SIZE - 1, 0).x;
            int py_left = CENTER_Y - project_point(3, BOARD_SIZE - 1, 0).y;
            int p1_x = px_left - SCALE * BOARD_SIZE / 2.0f + (i == 0 ? 0 : SCALE);
            int p1_y = py_left;
            int p2_x = px_left - SCALE * BOARD_SIZE / 2.0f - (i == BOARD_SIZE - 2 ? 0 : SCALE);
            int p2_y = py_left;
            DrawLine(p1_x, p1_y, p2_x, p2_y);
        }

        // 後面のグリッド線（z 方向、裏側）
        for (int i = 0; i < BOARD_SIZE - 1; ++i) {
            int px_back = CENTER_X + project_point(BOARD_SIZE, BOARD_SIZE - 1, i).x;
            int py_back = CENTER_Y - project_point(3, BOARD_SIZE - 1, i).y;
            int p1_x = px_back + SCALE * BOARD_SIZE / 2.0f;
            int p1_y = py_back + (i == 0 ? 0 : -SCALE);
            int p2_x = px_back + SCALE * BOARD_SIZE / 2.0f;
            int p2_y = py_back + (i == BOARD_SIZE - 2 ? 0 : -SCALE);
            DrawLine(p1_x, p1_y, p2_x, p2_y);
        }
    }

    void draw_face(int face, float angle) const {
        // 面の中心角を計算（等角投影）
        float cx = cosf(angle) * SCALE;
        float cy = -sinf(angle) * SCALE / 2.0f;
        int center_x = static_cast<int>(cx * BOARD_SIZE / 2.0f + CENTER_X);
        int center_y = static_cast<int>(CENTER_Y - cy * BOARD_SIZE / 2.0f);

        // ワイヤーライン（面の枠）
        float thickness = (face % 2 == 0) ? 1.0f : 1.5f;   // 前後は細く、左右は太く
        DrawLine(center_x - SCALE * BOARD_SIZE / 2.0f, center_y,
                 center_x + SCALE * BOARD_SIZE / 2.0f, center_y, thickness);
        DrawLine(center_x, center_y - SCALE * BOARD_SIZE / 4.0f, center_x, center_y + SCALE * BOARD_SIZE / 4.0f, thickness);

        // 石の描画（円形＋明度グラデーション）
        for (int x = 0; x < BOARD_SIZE; ++x) {
            for (int y = 0; y < BOARD_SIZE; ++y) {
                int px = center_x + project_point(x, y, 3).x;   // z=3（中心面）を表示
                int py = center_y - project_point(3, y, 3).y;

                PieceColor color = board_[x][y][3];   // z=3 面の石色

                if (color == PieceColor::EMPTY) continue;

                // 明度グラデーション：z が小さいほど暗く、大きいほど明るく
                float brightness = static_cast<float>(4 - z_of_face(face)) / 4.0f * 255.f;
                int gray = (color == PieceColor::BLACK) ?
                          static_cast<int>(60 + (brightness * 30.f / 255.f)) :
                          static_cast<int>(240 - brightness);

                // 黒石：暗い茶色（グラデーション）
                if (color == PieceColor::BLACK) {
                    DrawCircleFilled(px, py, 10, RGB(gray + 30, gray + 15, 40));   // 茶色系
                    // 「+」マークの簡易描画（空いている場合）
                    if (is_valid_move_at(x, y)) {
                        DrawTextAt(px - 8, py - 16, "+", RGB(200, 240, 200));
                    }
                } else {   // WHITE: 白系
                    DrawCircleFilled(px, py, 9, RGB(gray, gray, gray));
                    if (is_valid_move_at(x, y)) {
                        DrawTextAt(px - 8, py - 16, "+", RGB(200, 240, 200));
                    }
                }

                // z 軸による明度変化（立体感）
                int z_index = (face == 3) ? BOARD_SIZE - 1 : (BOARD_SIZE / 2);   // 簡易：中心面を明るく
                float depth_factor = static_cast<float>(z_index) / (BOARD_SIZE - 1.0f);
                if (depth_factor > 0.5f && color == PieceColor::BLACK) {
                    DrawCircle(px, py, 4, RGB(gray + 20, gray + 10, 30));   // ハイライト（内側）
                } else if (depth_factor < 0.5f && color == PieceColor::WHITE) {
                    DrawCircle(px - 2, py + 2, 4, RGB(gray + 10, gray + 10, gray + 10));   // シャドウ（外側）
                }
            }
        }
    }

    bool is_valid_move_at(int x, int y) const {
        for (int z = 0; z < BOARD_SIZE; ++z) {
            if (!board_[x][y][z]) continue;   // 埋まっているなら置けない
            auto flipped = board_.flip_stones(x, y, z, PieceColor::BLACK);
            if (!flipped.empty()) return true;
        }
        return false;
    }

    void draw_pieces() const {
        for (int x = 0; x < BOARD_SIZE; ++x) {
            for (int y = 0; y < BOARD_SIZE; ++y) {
                PieceColor color = board_[x][y][3];   // z=3（中心面）のみを表示
                if (color == PieceColor::EMPTY) continue;

                int px = CENTER_X + project_point(x, y, 3).x;
                int py = CENTER_Y - project_point(3, y, 3).y;

                PieceColor color2 = board_[x][y][0];   // z=0（前面）の石色も考慮
                if (color2 == PieceColor::BLACK) {
                    DrawCircleFilled(px, py, 10, RGB(65, 45, 25));   // 暗い茶色
                } else if (color2 == PieceColor::WHITE) {
                    DrawCircleFilled(px, py, 9, RGB(235, 235, 235));   // 白＋少しグレー
                }

                // 「+」マークの描画（有効手の場合）
                if (is_valid_move_at(x, y)) {
                    DrawTextAt(px - 8, py - 16, "+", RGB(100, 220, 100));
                }

                // z 軸による明度変化（立体感表現）
                float brightness = (color == PieceColor::BLACK) ? 70.0f : 245.0f;
                DrawCircle(px - 3, py + 3, 8, RGB(static_cast<int>(brightness), static_cast<int>(brightness), static_cast<int>(brightness)));   // ハイライト
            }
        }
    }

    void update_ui_panel() const {
        if (!dxlib_available_) return;

        // UI パネル背景（右下）
        DrawFilledRectangle(0, SCREEN_H - 48, SCREEN_W, 48, RGB(35, 45, 65));
        DrawTextAt(12, SCREEN_H - 18, "Cube Othello v0.1", RGB(200, 200, 200), 16);

        // ターン表示
        int turn_color = (get_turn_player() == BLACK) ? RGB(255, 230, 180) : RGB(180, 200, 255);
        DrawTextAt(12, SCREEN_H - 4, "Turn: ", RGB(200, 200, 200), 16);
        std::string turn_str = (get_turn_player() == BLACK) ? "[BLACK]" : "[WHITE]";
        DrawTextAt(75, SCREEN_H - 4, turn_str.c_str(), turn_color, 16);

        // スコア表示
        int black_count = count_pieces(PieceColor::BLACK);
        int white_count = count_pieces(PieceColor::WHITE);
        std::string score_str = "Score: B=" + std::to_string(black_count) + "/W=" + std::to_string(white_count);
        DrawTextAt(12, SCREEN_H - 22, score_str.c_str(), RGB(200, 200, 200), 16);

        // ゲーム終了時の表示
        if (is_game_over()) {
            char winner = get_winner();
            std::string result = "Winner: " + (winner == 'B' ? "BLACK" : (winner == 'W' ? "WHITE" : "DRAW"));
            DrawTextAt(12, SCREEN_H - 36, result.c_str(), RGB(255, 100, 80), 20);
        }
    }

public:
    // Board クラスからのメソッド参照（簡易なインターフェース）
    static int get_turn_player() { return 0; }   // プレイヤークラスから渡す必要があるが、簡易版では無視
    static bool is_game_over() { return false; }
    static PieceColorPieceColor count_pieces(PieceColor color) { return 0; }

    // 簡易インターフェース：実際の使用時は GameEngine から呼び出す
    void draw_with_engine(GameEngine& engine) const {
        if (!dxlib_available_) {
            auto terminal_output = render_terminal();
            printf("%s\n", terminal_output.c_str());
            return;
        }

        DrawFilledRectangle(0, 0, screen_width_, screen_height_, RGB(15, 20, 35));

        // ゲーム状態から盤面を取得して描画
        const auto& board = engine.get_board();
        draw_wireframe_for_engine(engine);   // ワイヤーフレーム（簡易：全体を黒い枠）
        draw_pieces_with_engine(board, engine);
    }
};

// ============================================================================
class GameGUI {
public:
    using BoardGrid = std::array<std::array<PieceColor, BOARD_SIZE>, BOARD_SIZE>;

private:
    CubeBoard board_;
    PieceColor turn_player_ = BLACK;   // 黒が先攻（black-on-white）
    bool game_over_ = false;
    char winner_ = '.';
    CuboGridRenderer renderer{board_.board_};   // コンストラクタで初期化

public:
    GameGUI() : board_{} {
        board_.initialize();   // 初期配置（黒・白を隣り合うように配置）
    }

    int main_loop() {
        if (!dxlib_available_) {
            auto output = renderer.render_terminal();
            printf("%s\n", output.c_str());
            return EXIT_SUCCESS;
        }

        // DxLib メインループ
        while (true) {   // 簡易：終了条件はゲームオーバーのみ（実際の DxLib では DrawWaitInputRet で待機）
            draw();

            if (is_game_over()) break;

            // キーボード入力処理
            handle_input();
        }

        return EXIT_SUCCESS;
    }

private:
    void draw() const {
        renderer.draw_dxlib();   // DxLib モード：ワイヤーフレーム＋石の描画＋有効手表示
    }

    void handle_input() {
        if (game_over_) return;

        // キーボード入力（簡易）
        int key = GetAsyncKeyState(VK_ESCAPE) ? VK_ESCAPE : 0;
        if (key == VK_ESCAPE) {
            // アプリ終了
            return;
        }

        // 有効手の選択（キー入力またはマウスクリック）
        auto valid_moves = engine.get_valid_moves();   // GameEngine から取得
        if (!valid_moves.empty()) {
            int move_x, move_y, move_z;
            // キー入力で座標を決定（簡易：数字入力）
            char input[16];
            GetKeyboardBuffer(input);

            // 簡易パース：キーボードからの数値入力を受け付ける場合、
            //   '1'...'7','8' → x, y, z の指定
            if (input[0] >= '1' && input[0] <= '9') {
                int digit = input[0] - '1';
                move_x = 3 + (digit / 2);   // x: 3 または 4
                move_y = 3 + (digit % 2);    // y: 3 または 4
                move_z = 3;                   // z=3（中心面）をデフォルト
            } else {
                // マウスクリック処理（簡易：座標→盤面マスの変換）
                int mouse_x, mouse_y;
                GetMousePosition(&mouse_x, &mouse_y);

                float angle = M_PI_F / 4.0f;   // 等角投影の角度
                D3DPoint projected = project_point(mouse_x, mouse_y);
                move_x = static_cast<int>((projected.x - CENTER_X) * BOARD_SIZE / SCALE + BOARD_SIZE / 2);
                move_y = static_cast<int>(((CENTER_Y - projected.y) * BOARD_SIZE / (SCALE * 2)) + BOARD_SIZE / 2);
            }

            // ゲームプレイの実行
            auto result = engine.play_move(move_x, move_y, move_z);
        }
    }
};

} // namespace cubo
