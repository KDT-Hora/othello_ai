#include "display.hpp"

#include <DxLib.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace cubo_othello {

namespace {
constexpr float kSpacing = 44.0f;    // pixels between neighbouring cell centers (before perspective)
constexpr float kCamDist = 22.0f;    // perspective camera distance, in cell units
constexpr float kCenter = (BOARD_SIZE - 1) * 0.5f;
constexpr float kPi = 3.14159265f;
constexpr float kKeyRotateStep = 0.03f;
constexpr float kDragRotatePerPixel = 0.008f;
constexpr float kDefaultYaw = 0.6f;
constexpr float kDefaultPitch = 0.5f;

unsigned int color_black_stone() { return GetColor(0x2a, 0x36, 0x4a); }
unsigned int color_white_stone() { return GetColor(0xf0, 0xf0, 0xf5); }
unsigned int color_grid_line() { return GetColor(0x34, 0x34, 0x4c); }
unsigned int color_frame_line() { return GetColor(0x88, 0x88, 0xb0); }
unsigned int color_marker() { return GetColor(0x30, 0xff, 0x60); }
unsigned int color_hover() { return GetColor(0xff, 0xe0, 0x40); }
unsigned int color_text() { return GetColor(0xff, 0xff, 0xff); }

int shade(int c, float k) { return std::clamp(static_cast<int>(static_cast<float>(c) * k), 0, 255); }
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

void DXLibDisplay::project(float px, float py, float pz, float& sx, float& sy, float& depth,
                           float& scale) const {
    const float x = px - kCenter;
    const float y = py - kCenter;
    const float z = pz - kCenter;

    const float cyaw = std::cos(yaw_), syaw = std::sin(yaw_);
    const float x1 = x * cyaw + z * syaw;
    const float z1 = -x * syaw + z * cyaw;

    const float cp = std::cos(pitch_), sp = std::sin(pitch_);
    const float y2 = y * cp - z1 * sp;
    const float z2 = y * sp + z1 * cp;

    scale = kCamDist / (kCamDist + z2);
    depth = z2;
    sx = static_cast<float>(width_) * 0.5f + x1 * kSpacing * scale;
    sy = static_cast<float>(height_) * 0.5f + y2 * kSpacing * scale;
}

void DXLibDisplay::draw_cube_faces() {
    // Outline of the 8x8x8 block; cell boundaries lie at -0.5 .. 7.5.
    const float lo = -0.5f, hi = BOARD_SIZE - 0.5f;
    const float c[8][3] = {
        {lo, lo, lo}, {hi, lo, lo}, {hi, hi, lo}, {lo, hi, lo},
        {lo, lo, hi}, {hi, lo, hi}, {hi, hi, hi}, {lo, hi, hi},
    };
    const int edges[12][2] = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6}, {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7},
    };
    for (const auto& e : edges) {
        float x1, y1, d1, s1, x2, y2, d2, s2;
        project(c[e[0]][0], c[e[0]][1], c[e[0]][2], x1, y1, d1, s1);
        project(c[e[1]][0], c[e[1]][1], c[e[1]][2], x2, y2, d2, s2);
        DrawLine(static_cast<int>(x1), static_cast<int>(y1), static_cast<int>(x2), static_cast<int>(y2),
                 color_frame_line());
    }
}

void DXLibDisplay::draw_grid() {
    // Cell-boundary grid on the three faces farthest from the viewer, so
    // every cell reads as a square "masu" behind the stones.
    const float lo = -0.5f, hi = BOARD_SIZE - 0.5f;
    for (int axis = 0; axis < 3; ++axis) {
        float p[3] = {kCenter, kCenter, kCenter};
        float sx, sy, d_lo, d_hi, sc;
        p[axis] = lo;
        project(p[0], p[1], p[2], sx, sy, d_lo, sc);
        p[axis] = hi;
        project(p[0], p[1], p[2], sx, sy, d_hi, sc);
        const float face = (d_hi > d_lo) ? hi : lo; // farther side

        const int u = (axis + 1) % 3, v = (axis + 2) % 3;
        for (int k = 0; k <= BOARD_SIZE; ++k) {
            const float t = static_cast<float>(k) - 0.5f;
            for (int dir = 0; dir < 2; ++dir) {
                float a[3], b[3];
                a[axis] = b[axis] = face;
                const int fixed = (dir == 0) ? u : v;
                const int run = (dir == 0) ? v : u;
                a[fixed] = b[fixed] = t;
                a[run] = lo;
                b[run] = hi;
                float x1, y1, d1, s1, x2, y2, d2, s2;
                project(a[0], a[1], a[2], x1, y1, d1, s1);
                project(b[0], b[1], b[2], x2, y2, d2, s2);
                DrawLine(static_cast<int>(x1), static_cast<int>(y1), static_cast<int>(x2),
                         static_cast<int>(y2), color_grid_line());
            }
        }
    }
}

std::optional<Move> DXLibDisplay::pick_move(const CubeBoard& board, int8_t turn, int mx, int my) const {
    std::optional<Move> best;
    float best_depth = std::numeric_limits<float>::max();
    for (const auto& mv : board.valid_moves(turn)) {
        float sx, sy, depth, scale;
        project(static_cast<float>(mv.x), static_cast<float>(mv.y), static_cast<float>(mv.z), sx, sy, depth,
                scale);
        const float r = kSpacing * 0.3f * scale;
        const float dx = sx - static_cast<float>(mx), dy = sy - static_cast<float>(my);
        // Among overlapping candidates, prefer the one nearest the viewer.
        if (dx * dx + dy * dy <= r * r && depth < best_depth) {
            best_depth = depth;
            best = mv;
        }
    }
    return best;
}

void DXLibDisplay::draw_stones(const CubeBoard& board) {
    struct Cell {
        float depth;
        int x, y, z;
    };
    std::vector<Cell> cells;
    cells.reserve(BOARD_SIZE * BOARD_SIZE * BOARD_SIZE);
    for (int z = 0; z < BOARD_SIZE; ++z)
        for (int y = 0; y < BOARD_SIZE; ++y)
            for (int x = 0; x < BOARD_SIZE; ++x) {
                float sx, sy, d, sc;
                project(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z), sx, sy, d, sc);
                cells.push_back({d, x, y, z});
            }
    // Painter's algorithm: far cells first so near stones cover them.
    std::sort(cells.begin(), cells.end(), [](const Cell& a, const Cell& b) { return a.depth > b.depth; });

    for (const auto& c : cells) {
        float sxf, syf, depth, scale;
        project(static_cast<float>(c.x), static_cast<float>(c.y), static_cast<float>(c.z), sxf, syf, depth,
                scale);
        const int sx = static_cast<int>(sxf), sy = static_cast<int>(syf);
        const int r = std::max(2, static_cast<int>(kSpacing * 0.3f * scale));
        // Nearer = brighter, so depth stays readable where cells overlap.
        const float light = std::clamp(1.0f - depth / (kCenter * 3.4f) * 0.55f, 0.4f, 1.15f);

        const int8_t cell = board.at(c.x, c.y, c.z);
        if (cell == EMPTY) {
            DrawCircle(sx, sy, std::max(1, r / 4),
                       GetColor(shade(0x5a, light), shade(0x5a, light), shade(0x74, light)), TRUE);
            continue;
        }

        const unsigned int base = (cell == BLACK) ? color_black_stone() : color_white_stone();
        int cr, cg, cb;
        GetColor2(base, &cr, &cg, &cb);
        DrawCircle(sx, sy, r, GetColor(shade(cr, light), shade(cg, light), shade(cb, light)), TRUE);
        DrawCircle(sx, sy, r, (cell == BLACK) ? GetColor(0x90, 0xa0, 0xc0) : GetColor(0x30, 0x30, 0x40),
                   FALSE, 2);
        // Small specular dot so the stone reads as a sphere.
        DrawCircle(sx - r / 3, sy - r / 3, std::max(1, r / 5),
                   (cell == BLACK) ? GetColor(0x70, 0x80, 0xa0) : GetColor(255, 255, 255), TRUE);
    }
}

void DXLibDisplay::mark_valid_moves(const CubeBoard& board, int8_t color) {
    int mx, my;
    GetMousePoint(&mx, &my);
    const auto hover = pick_move(board, color, mx, my);

    for (const auto& mv : board.valid_moves(color)) {
        float sx, sy, depth, scale;
        project(static_cast<float>(mv.x), static_cast<float>(mv.y), static_cast<float>(mv.z), sx, sy, depth,
                scale);
        const int px = static_cast<int>(sx), py = static_cast<int>(sy);
        const int r = std::max(3, static_cast<int>(kSpacing * 0.3f * scale));
        const bool hot = hover && hover->x == mv.x && hover->y == mv.y && hover->z == mv.z;
        if (hot) {
            DrawCircle(px, py, r + 3, color_hover(), FALSE, 3);
            DrawCircle(px, py, r / 2, color_hover(), TRUE);
        } else {
            DrawCircle(px, py, r, color_marker(), FALSE, 2);
        }
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
    DrawFormatString(8, height_ - 52, color_text(),
                      "Right-drag / Arrows: rotate view   V: reset view   (green ring = legal move)");
    DrawFormatString(8, height_ - 36, color_text(), "Click: place   R: reset   U: undo   ESC: quit");
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

    // View rotation: arrow keys and right-button drag.
    if (CheckHitKey(KEY_INPUT_LEFT)) yaw_ -= kKeyRotateStep;
    if (CheckHitKey(KEY_INPUT_RIGHT)) yaw_ += kKeyRotateStep;
    if (CheckHitKey(KEY_INPUT_UP)) pitch_ -= kKeyRotateStep;
    if (CheckHitKey(KEY_INPUT_DOWN)) pitch_ += kKeyRotateStep;
    if (CheckHitKey(KEY_INPUT_V)) {
        yaw_ = kDefaultYaw;
        pitch_ = kDefaultPitch;
    }

    int mx, my;
    GetMousePoint(&mx, &my);
    const int buttons = GetMouseInput();

    const bool right_down = (buttons & MOUSE_INPUT_RIGHT) != 0;
    if (right_down && right_was_down_) {
        yaw_ += static_cast<float>(mx - last_mouse_x_) * kDragRotatePerPixel;
        pitch_ += static_cast<float>(my - last_mouse_y_) * kDragRotatePerPixel;
    }
    right_was_down_ = right_down;
    last_mouse_x_ = mx;
    last_mouse_y_ = my;

    if (yaw_ > kPi) yaw_ -= 2 * kPi;
    if (yaw_ < -kPi) yaw_ += 2 * kPi;
    pitch_ = std::clamp(pitch_, -1.5f, 1.5f);

    // Place a stone once per left click (not repeatedly while held).
    const bool left_down = (buttons & MOUSE_INPUT_LEFT) != 0;
    if (left_down && !left_was_down_) {
        result.clicked_move = pick_move(board, turn, mx, my);
    }
    left_was_down_ = left_down;

    return result;
}

} // namespace cubo_othello
