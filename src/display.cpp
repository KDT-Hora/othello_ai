#include "display.hpp"

#include <DxLib.h>

#include <algorithm>
#include <cmath>
#include <limits>

namespace cubo_othello {

namespace {
constexpr float kGS = 46.0f;      // pixels per grid unit
constexpr float kOriginPxMid = 0.8f;
constexpr float kOriginPyMid = 2.7f;

unsigned int color_black_stone() { return GetColor(0x3a, 0x4a, 0x5e); }
unsigned int color_white_stone() { return GetColor(0xf0, 0xf0, 0xf5); }
unsigned int color_empty_cell() { return GetColor(0x60, 0x60, 0x70); }
unsigned int color_grid_line() { return GetColor(0x40, 0x40, 0x55); }
unsigned int color_marker() { return GetColor(0x30, 0xff, 0x60); }
unsigned int color_text() { return GetColor(0xff, 0xff, 0xff); }
} // namespace

bool DXLibDisplay::init(int width, int height) {
    width_ = width;
    height_ = height;

    SetOutApplicationLogValidFlag(FALSE); // must precede other DxLib calls to suppress Log.txt
    SetGraphMode(width_, height_, 32);
    ChangeWindowMode(TRUE);
    SetWindowText("Cube Othello");

    if (DxLib_Init() == -1) return false;

    SetBackgroundColor(0x1C, 0x1C, 0x2A);
    SetDrawScreen(DX_SCREEN_BACK);
    return true;
}

void DXLibDisplay::shutdown() { DxLib_End(); }

void DXLibDisplay::project(int x, int y, int z, int& sx, int& sy) const {
    const float px = x * 0.5f - y * 0.268f;
    const float py = y * 0.5f + z * 0.268f;
    sx = static_cast<int>(width_ * 0.5f + (px - kOriginPxMid) * kGS);
    sy = static_cast<int>(height_ * 0.42f + (py - kOriginPyMid) * kGS);
}

float DXLibDisplay::lighting(int z) const {
    return std::max(0.3f, 1.0f - static_cast<float>(z) / 7.0f * 0.5f);
}

void DXLibDisplay::draw_cube_faces() {
    // Wireframe outline of the three visible faces (front, top, right),
    // conveying pseudo-3D structure without true occlusion.
    int corners[8][3] = {
        {0, 0, 0}, {7, 0, 0}, {7, 7, 0}, {0, 7, 0},
        {0, 0, 7}, {7, 0, 7}, {7, 7, 7}, {0, 7, 7},
    };
    int edges[12][2] = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0},
        {4, 5}, {5, 6}, {6, 7}, {7, 4},
        {0, 4}, {1, 5}, {2, 6}, {3, 7},
    };
    for (auto& e : edges) {
        int x1, y1, x2, y2;
        project(corners[e[0]][0], corners[e[0]][1], corners[e[0]][2], x1, y1);
        project(corners[e[1]][0], corners[e[1]][1], corners[e[1]][2], x2, y2);
        DrawLine(x1, y1, x2, y2, color_grid_line());
    }
}

void DXLibDisplay::draw_grid() {
    for (int i = 0; i <= 7; ++i) {
        int x1, y1, x2, y2;
        project(i, 0, 0, x1, y1);
        project(i, 7, 0, x2, y2);
        DrawLine(x1, y1, x2, y2, color_grid_line());

        project(0, i, 0, x1, y1);
        project(7, i, 0, x2, y2);
        DrawLine(x1, y1, x2, y2, color_grid_line());
    }
}

void DXLibDisplay::draw_stones(const CubeBoard& board) {
    const int radius = static_cast<int>(kGS * 0.32f);

    for (int z = 0; z < BOARD_SIZE; ++z) {
        for (int y = 0; y < BOARD_SIZE; ++y) {
            for (int x = 0; x < BOARD_SIZE; ++x) {
                int8_t cell = board.at(x, y, z);
                int sx, sy;
                project(x, y, z, sx, sy);
                const float light = lighting(z);

                if (cell == EMPTY) {
                    DrawOval(sx, sy, radius / 3, radius / 3, color_empty_cell(), TRUE);
                    continue;
                }

                unsigned int base = (cell == BLACK) ? color_black_stone() : color_white_stone();
                int r, g, b;
                GetColor2(base, &r, &g, &b);
                unsigned int shaded = GetColor(static_cast<int>(r * light), static_cast<int>(g * light),
                                                static_cast<int>(b * light));
                DrawOval(sx, sy, radius, radius, shaded, TRUE);
                DrawOval(sx, sy, radius, radius, GetColor(0, 0, 0), FALSE);
            }
        }
    }
}

void DXLibDisplay::mark_valid_moves(const CubeBoard& board, int8_t color) {
    for (const auto& mv : board.valid_moves(color)) {
        int sx, sy;
        project(mv.x, mv.y, mv.z, sx, sy);
        DrawFormatString(sx - 4, sy - static_cast<int>(kGS * 0.32f) - 12, color_marker(), "+");
    }
}

void DXLibDisplay::render_frame(const CubeBoard& board, int8_t turn, const std::string& status_message) {
    ClearDrawScreen();

    draw_cube_faces();
    draw_grid();
    draw_stones(board);
    mark_valid_moves(board, turn);

    const char* turn_label = (turn == BLACK) ? "BLACK" : "WHITE";
    DrawFormatString(8, 8, color_text(), "Turn: %s   B:%d  W:%d", turn_label, board.count(BLACK),
                      board.count(WHITE));
    DrawFormatString(8, height_ - 36, color_text(), "R: reset   U: undo   ESC: quit");
    if (!status_message.empty()) {
        DrawFormatString(8, height_ - 20, color_text(), "%s", status_message.c_str());
    }

    ScreenFlip();
}

InputResult DXLibDisplay::handle_input(const CubeBoard& board, int8_t turn) {
    InputResult result;

    if (ProcessMessage() == -1) {
        result.quit = true;
        return result;
    }

    if (CheckHitKey(KEY_INPUT_ESCAPE)) {
        result.quit = true;
        return result;
    }
    if (CheckHitKey(KEY_INPUT_R)) {
        result.reset_requested = true;
    }
    const bool undo_key_down = CheckHitKey(KEY_INPUT_U) != 0;
    if (undo_key_down && !undo_key_was_down_) {
        result.undo_requested = true;
    }
    undo_key_was_down_ = undo_key_down;

    if ((GetMouseInput() & MOUSE_INPUT_LEFT) != 0) {
        int mx, my;
        GetMousePoint(&mx, &my);

        auto moves = board.valid_moves(turn);
        float best_dist = std::numeric_limits<float>::max();
        std::optional<Move> best;

        for (const auto& mv : moves) {
            int sx, sy;
            project(mv.x, mv.y, mv.z, sx, sy);
            float dx = static_cast<float>(sx - mx);
            float dy = static_cast<float>(sy - my);
            float dist = dx * dx + dy * dy;
            if (dist < best_dist) {
                best_dist = dist;
                best = mv;
            }
        }

        constexpr float kClickRadius = 22.0f;
        if (best && best_dist <= kClickRadius * kClickRadius) {
            result.clicked_move = best;
        }
    }

    return result;
}

} // namespace cubo_othello
