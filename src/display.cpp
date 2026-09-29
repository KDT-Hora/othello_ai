#include "display.hpp"

#include <DxLib.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <vector>

namespace cubo_othello {

namespace {
constexpr float kSpacing = 60.0f;    // pixels between neighbouring cell centers (before perspective)
constexpr float kCamDist = 22.0f;    // perspective camera distance, in cell units
constexpr float kStoneRadius = 0.36f; // stone radius as a fraction of cell spacing
constexpr float kCenter = (BOARD_SIZE - 1) * 0.5f;
constexpr float kPi = 3.14159265f;
constexpr float kKeyRotateStep = 0.03f;
constexpr float kDragRotatePerPixel = 0.008f;
constexpr float kDefaultYaw = 0.6f;
constexpr float kDefaultPitch = 0.5f;

// Cyberpunk neon palette.
constexpr int kCyan[3] = {0, 229, 255};
constexpr int kMagenta[3] = {255, 43, 214};
constexpr int kNeonGreen[3] = {57, 255, 20};
constexpr int kYellow[3] = {255, 240, 60};

unsigned int rgb(const int (&c)[3], float k = 1.0f) {
    return GetColor(std::clamp(static_cast<int>(static_cast<float>(c[0]) * k), 0, 255),
                    std::clamp(static_cast<int>(static_cast<float>(c[1]) * k), 0, 255),
                    std::clamp(static_cast<int>(static_cast<float>(c[2]) * k), 0, 255));
}
unsigned int color_grid_line() { return GetColor(0x26, 0x3a, 0x66); }
unsigned int color_dim_text() { return GetColor(0x6a, 0x8a, 0xb8); }

int shade(int c, float k) { return std::clamp(static_cast<int>(static_cast<float>(c) * k), 0, 255); }

// Additive soft glow: a few concentric translucent discs.
void glow(int x, int y, int r, const int (&c)[3], int strength) {
    SetDrawBlendMode(DX_BLENDMODE_ADD, std::clamp(strength, 0, 255));
    for (int i = 3; i >= 1; --i) DrawCircle(x, y, r + i * std::max(2, r / 3), rgb(c, 0.35f), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

// Neon line: wide dim additive stroke under a thin bright core.
void neon_line(int x1, int y1, int x2, int y2, const int (&c)[3], int alpha) {
    SetDrawBlendMode(DX_BLENDMODE_ADD, alpha);
    DrawLine(x1, y1, x2, y2, rgb(c, 0.6f), 5);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawLine(x1, y1, x2, y2, rgb(c), 1);
}

void draw_background(int w, int h) {
    for (int y = 0; y < h; ++y) {
        const float t = static_cast<float>(y) / static_cast<float>(h);
        DrawLine(0, y, w, y, GetColor(static_cast<int>(8 + 30 * t * t), 5, static_cast<int>(22 + 40 * t)));
    }
    // CRT scanlines.
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 45);
    for (int y = 0; y < h; y += 3) DrawLine(0, y, w, y, GetColor(0, 0, 0));
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

int text_width(const char* text, int size) {
    SetFontSize(size);
    const int w = GetDrawStringWidth(text, static_cast<int>(std::strlen(text)));
    SetFontSize(16);
    return w;
}

void text_glow(int x, int y, const char* text, const int (&c)[3], int size) {
    SetFontSize(size);
    SetDrawBlendMode(DX_BLENDMODE_ADD, 90);
    DrawString(x - 1, y, text, rgb(c, 0.6f));
    DrawString(x + 1, y, text, rgb(c, 0.6f));
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    DrawString(x, y, text, rgb(c));
    SetFontSize(16);
}
} // namespace

bool DXLibDisplay::init(int width, int height) {
    width_ = width;
    height_ = height;

    SetOutApplicationLogValidFlag(FALSE); // must precede other DxLib calls to suppress Log.txt
    SetGraphMode(width_, height_, 32);
    ChangeWindowMode(TRUE);
    SetWindowText("Cube Othello");

    SetUseCharCodeFormat(DX_CHARCODEFORMAT_UTF8); // source strings are UTF-8 (MSVC /utf-8)
    if (DxLib_Init() == -1) return false;
    ChangeFont("MS Gothic");

    SetBackgroundColor(0x08, 0x05, 0x16);
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
    // Outline of the cube; cell boundaries lie at -0.5 .. BOARD_SIZE-0.5.
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
        neon_line(static_cast<int>(x1), static_cast<int>(y1), static_cast<int>(x2), static_cast<int>(y2),
                  kMagenta, 60);
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
        const float r = kSpacing * kStoneRadius * scale;
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
        const int r = std::max(2, static_cast<int>(kSpacing * kStoneRadius * scale));
        // Nearer = brighter, so depth stays readable where cells overlap.
        const float light = std::clamp(1.0f - depth / (kCenter * 3.4f) * 0.8f, 0.25f, 1.15f);

        const int8_t cell = board.at(c.x, c.y, c.z);
        if (cell == EMPTY) {
            DrawCircle(sx, sy, std::max(1, r / 7),
                       GetColor(shade(0x38, light), shade(0x4c, light), shade(0x78, light)), TRUE);
            continue;
        }

        // BLACK = dark core with cyan neon; WHITE = bright core with magenta neon.
        const bool is_black = (cell == BLACK);
        const auto& neon = is_black ? kCyan : kMagenta;
        glow(sx, sy, r, neon, static_cast<int>(40.0f * light));
        if (is_black) {
            DrawCircle(sx, sy, r, GetColor(shade(12, light), shade(18, light), shade(40, light)), TRUE);
        } else {
            DrawCircle(sx, sy, r, GetColor(shade(255, light), shade(235, light), shade(255, light)), TRUE);
        }
        DrawCircle(sx, sy, r, rgb(neon, light), FALSE, 2);
        // Specular highlight so the stone reads as a sphere.
        DrawCircle(sx - r / 3, sy - r / 3, std::max(1, r / 5),
                   is_black ? rgb(kCyan, 0.7f * light) : GetColor(255, 255, 255), TRUE);
    }
}

void DXLibDisplay::mark_valid_moves(const CubeBoard& board, int8_t color) {
    int mx, my;
    GetMousePoint(&mx, &my);
    const bool human_turn = !spectate_ && !(ai_color_ && *ai_color_ == color);
    const auto hover = human_turn ? pick_move(board, color, mx, my) : std::nullopt;
    hover_flip_count_ = -1;
    const float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(GetNowCount()) * 0.006f);

    for (const auto& mv : board.valid_moves(color)) {
        float sx, sy, depth, scale;
        project(static_cast<float>(mv.x), static_cast<float>(mv.y), static_cast<float>(mv.z), sx, sy, depth,
                scale);
        const int px = static_cast<int>(sx), py = static_cast<int>(sy);
        const int r = std::max(3, static_cast<int>(kSpacing * kStoneRadius * scale));
        const bool hot = hover && hover->x == mv.x && hover->y == mv.y && hover->z == mv.z;
        if (hot) {
            // Preview: mark every stone this move would flip.
            const auto flips = board.flips_for(mv.x, mv.y, mv.z, color);
            hover_flip_count_ = static_cast<int>(flips.size());
            const auto& mover_neon = (color == BLACK) ? kCyan : kMagenta;
            for (const auto& f : flips) {
                float fx, fy, fd, fs;
                project(static_cast<float>(f.x), static_cast<float>(f.y), static_cast<float>(f.z), fx, fy, fd, fs);
                const int qx = static_cast<int>(fx), qy = static_cast<int>(fy);
                const int fr = std::max(3, static_cast<int>(kSpacing * kStoneRadius * fs));
                SetDrawBlendMode(DX_BLENDMODE_ADD, 110);
                DrawLine(px, py, qx, qy, rgb(kYellow), 3);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                glow(qx, qy, fr, kYellow, static_cast<int>(20.0f + 30.0f * pulse));
                DrawCircle(qx, qy, fr + 5, rgb(kYellow), FALSE, 4);
                DrawCircle(qx, qy, fr + 5, GetColor(0, 0, 0), FALSE, 1);
                DrawCircle(qx, qy, std::max(2, fr / 3), rgb(mover_neon), TRUE); // the color it will become
            }
            glow(px, py, r, kYellow, 140);
            DrawCircle(px, py, r + 3, rgb(kYellow), FALSE, 3);
            DrawCircle(px, py, r / 2, rgb(kYellow), TRUE);
        } else {
            glow(px, py, r, kNeonGreen, static_cast<int>(25.0f + 40.0f * pulse));
            DrawCircle(px, py, r, rgb(kNeonGreen), FALSE, 2);
        }
    }
}

void DXLibDisplay::render_frame(const CubeBoard& board, int8_t turn, const std::string& status_message) {
    ClearDrawScreen();
    draw_background(width_, height_);

    draw_cube_faces();
    draw_grid();
    draw_stones(board);
    mark_valid_moves(board, turn);

    // --- HUD -------------------------------------------------------------
    text_glow(20, 14, "キューブ・オセロ", kCyan, 28);
    DrawLine(20, 50, 300, 50, rgb(kCyan, 0.6f));

    const bool black_turn = (turn == BLACK);
    char buf[96];
    const char* who = spectate_ ? "  ＜AI同士で観戦中＞"
                      : (!ai_color_ ? "" : (*ai_color_ == turn ? "  ＜AIの番＞" : "  ＜あなたの番＞"));
    std::snprintf(buf, sizeof buf, "手番： %s%s", black_turn ? "黒（シアン）" : "白（マゼンタ）", who);
    text_glow(20, 60, buf, black_turn ? kCyan : kMagenta, 22);

    const int legal = static_cast<int>(board.valid_moves(turn).size());
    std::snprintf(buf, sizeof buf, "置ける場所： %d か所（緑の輪）", legal);
    DrawString(20, 92, buf, rgb(kNeonGreen));
    if (hover_flip_count_ >= 0) {
        std::snprintf(buf, sizeof buf, "ここに置くと %d 個ひっくり返る（黄色の輪）", hover_flip_count_);
        DrawString(20, 114, buf, rgb(kYellow));
    } else if (!spectate_) {
        DrawString(20, 114, "緑の輪にカーソルを合わせると、裏返る石が分かります", color_dim_text());
    }

    const int b = board.count(BLACK), w = board.count(WHITE);
    const int bar_w = 260, bar_x = width_ - bar_w - 24, bar_y = 40;
    const int total = std::max(1, b + w);
    const int b_px = bar_w * b / total;
    DrawString(bar_x, 14, "石の数", color_dim_text());
    DrawBox(bar_x - 2, bar_y - 2, bar_x + bar_w + 2, bar_y + 16, GetColor(10, 10, 30), TRUE);
    DrawBox(bar_x, bar_y, bar_x + b_px, bar_y + 14, rgb(kCyan, 0.85f), TRUE);
    DrawBox(bar_x + b_px, bar_y, bar_x + bar_w, bar_y + 14, rgb(kMagenta, 0.85f), TRUE);
    DrawBox(bar_x - 2, bar_y - 2, bar_x + bar_w + 2, bar_y + 16, rgb(kCyan, 0.7f), FALSE);
    std::snprintf(buf, sizeof buf, "黒 %d", b);
    DrawString(bar_x, bar_y + 22, buf, rgb(kCyan));
    std::snprintf(buf, sizeof buf, "白 %d", w);
    DrawString(bar_x + bar_w - GetDrawStringWidth(buf, static_cast<int>(std::strlen(buf))), bar_y + 22, buf,
               rgb(kMagenta));

    DrawString(20, height_ - 62, "視点回転： 右ドラッグ / 矢印キー　　視点リセット： V", color_dim_text());
    DrawString(20, height_ - 42, "石を置く： 左クリック　　一手戻す： U　　最初から： R　　メニュー： M　　終了： ESC", color_dim_text());
    DrawString(20, height_ - 22, "ルール： 縦・横・斜め（26方向）に相手の石を挟むと裏返せます", color_dim_text());

    if (!status_message.empty()) {
        const int tw = text_width(status_message.c_str(), 32);
        text_glow(width_ / 2 - tw / 2, height_ / 2 - 20, status_message.c_str(), kYellow, 32);
    }

    ScreenFlip();
}

namespace {
constexpr int kMenuRowW = 520, kMenuRowH = 50, kMenuRowGap = 10, kMenuRowTop = 168;
constexpr int kMenuDiffW = 150, kMenuDiffH = 44, kMenuDiffGap = 20;
constexpr int kMenuStartW = 300, kMenuStartH = 52;

const char* const kModeNames[kMenuModeCount] = {"二人で対戦", "AIと対戦　あなたは黒（先手）",
                                                 "AIと対戦　あなたは白（後手）", "AI同士で観戦"};
const char* const kDiffNames[kMenuDifficultyCount] = {"かんたん", "ふつう", "むずかしい"};

bool in_box(int mx, int my, int x, int y, int w, int h) { return mx >= x && mx < x + w && my >= y && my < y + h; }
int str_w(const char* s) { return GetDrawStringWidth(s, static_cast<int>(std::strlen(s))); }
int diff_top() { return kMenuRowTop + kMenuModeCount * (kMenuRowH + kMenuRowGap) + 24; }
int diff_left(int cx, int i) {
    const int total = kMenuDifficultyCount * kMenuDiffW + (kMenuDifficultyCount - 1) * kMenuDiffGap;
    return cx - total / 2 + i * (kMenuDiffW + kMenuDiffGap);
}
int start_top() { return diff_top() + kMenuDiffH + 28; }
} // namespace

void DXLibDisplay::render_menu(int mode, int difficulty) {
    ClearDrawScreen();
    draw_background(width_, height_);

    int mx, my;
    GetMousePoint(&mx, &my);
    const int cx = width_ / 2;

    const char* title = "キューブ・オセロ";
    text_glow(cx - text_width(title, 48) / 2, 60, title, kCyan, 48);
    const char* sub = "モードを選んでください";
    DrawString(cx - str_w(sub) / 2, 130, sub, color_dim_text());

    for (int i = 0; i < kMenuModeCount; ++i) {
        const int x = cx - kMenuRowW / 2, y = kMenuRowTop + i * (kMenuRowH + kMenuRowGap);
        const bool sel = (i == mode);
        const bool hov = in_box(mx, my, x, y, kMenuRowW, kMenuRowH);
        DrawBox(x, y, x + kMenuRowW, y + kMenuRowH, sel ? GetColor(0, 50, 70) : GetColor(12, 10, 32), TRUE);
        if (sel) glow(cx, y + kMenuRowH / 2, kMenuRowH / 2, kCyan, 20);
        DrawBox(x, y, x + kMenuRowW, y + kMenuRowH,
                sel ? rgb(kCyan) : (hov ? rgb(kMagenta, 0.8f) : GetColor(60, 70, 110)), FALSE);
        SetFontSize(24);
        DrawString(cx - str_w(kModeNames[i]) / 2, y + 12, kModeNames[i],
                   sel ? rgb(kCyan) : GetColor(200, 210, 235));
        SetFontSize(16);
    }

    const bool ai_mode = (mode != 0);
    const int dy = diff_top();
    const char* dlabel = ai_mode ? "AIの強さ" : "AIの強さ（AI対戦のみ）";
    DrawString(cx - str_w(dlabel) / 2, dy - 26, dlabel, ai_mode ? rgb(kMagenta) : color_dim_text());
    for (int i = 0; i < kMenuDifficultyCount; ++i) {
        const int x = diff_left(cx, i);
        const bool sel = (i == difficulty);
        const bool hov = ai_mode && in_box(mx, my, x, dy, kMenuDiffW, kMenuDiffH);
        DrawBox(x, dy, x + kMenuDiffW, dy + kMenuDiffH,
                (sel && ai_mode) ? GetColor(60, 10, 50) : GetColor(12, 10, 32), TRUE);
        DrawBox(x, dy, x + kMenuDiffW, dy + kMenuDiffH,
                ai_mode ? (sel ? rgb(kMagenta) : (hov ? rgb(kMagenta, 0.7f) : GetColor(60, 70, 110)))
                        : GetColor(40, 45, 70),
                FALSE);
        DrawString(x + (kMenuDiffW - str_w(kDiffNames[i])) / 2, dy + 14, kDiffNames[i],
                   ai_mode ? (sel ? rgb(kMagenta) : GetColor(200, 210, 235)) : GetColor(70, 80, 110));
    }

    const int sy = start_top(), sx = cx - kMenuStartW / 2;
    const bool start_hov = in_box(mx, my, sx, sy, kMenuStartW, kMenuStartH);
    glow(cx, sy + kMenuStartH / 2, 40, kNeonGreen, start_hov ? 50 : 25);
    DrawBox(sx, sy, sx + kMenuStartW, sy + kMenuStartH, start_hov ? GetColor(10, 60, 10) : GetColor(8, 30, 12), TRUE);
    DrawBox(sx, sy, sx + kMenuStartW, sy + kMenuStartH, rgb(kNeonGreen), FALSE);
    const char* start = "ゲーム開始";
    SetFontSize(24);
    DrawString(cx - str_w(start) / 2, sy + 13, start, rgb(kNeonGreen));
    SetFontSize(16);

    const char* help = "↑↓：モード選択　　←→：AIの強さ　　Enter：開始　　ESC：終了";
    DrawString(cx - str_w(help) / 2, height_ - 30, help, color_dim_text());

    ScreenFlip();
}

MenuInput DXLibDisplay::handle_menu_input() {
    MenuInput r;
    if (ProcessMessage() == -1 || CheckHitKey(KEY_INPUT_ESCAPE)) {
        r.quit = true;
        return r;
    }

    const int keys[5] = {KEY_INPUT_UP, KEY_INPUT_DOWN, KEY_INPUT_LEFT, KEY_INPUT_RIGHT, KEY_INPUT_RETURN};
    bool down[5];
    for (int i = 0; i < 5; ++i) down[i] = CheckHitKey(keys[i]) != 0;
    down[4] = down[4] || CheckHitKey(KEY_INPUT_SPACE) != 0;
    if (down[0] && !menu_key_prev_[0]) r.move = -1;
    if (down[1] && !menu_key_prev_[1]) r.move = +1;
    if (down[2] && !menu_key_prev_[2]) r.diff_move = -1;
    if (down[3] && !menu_key_prev_[3]) r.diff_move = +1;
    if (down[4] && !menu_key_prev_[4]) r.confirm = true;
    for (int i = 0; i < 5; ++i) menu_key_prev_[i] = down[i];

    const bool click = (GetMouseInput() & MOUSE_INPUT_LEFT) != 0;
    if (click && !menu_click_prev_) {
        int mx, my;
        GetMousePoint(&mx, &my);
        const int cx = width_ / 2;
        for (int i = 0; i < kMenuModeCount; ++i) {
            if (in_box(mx, my, cx - kMenuRowW / 2, kMenuRowTop + i * (kMenuRowH + kMenuRowGap), kMenuRowW, kMenuRowH))
                r.mode_row = i;
        }
        for (int i = 0; i < kMenuDifficultyCount; ++i) {
            if (in_box(mx, my, diff_left(cx, i), diff_top(), kMenuDiffW, kMenuDiffH)) r.difficulty_col = i;
        }
        if (in_box(mx, my, cx - kMenuStartW / 2, start_top(), kMenuStartW, kMenuStartH)) r.confirm = true;
    }
    menu_click_prev_ = click;
    return r;
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
    const bool menu_key_down = CheckHitKey(KEY_INPUT_M) != 0;
    if (menu_key_down && !menu_key_was_down_) result.menu_requested = true;
    menu_key_was_down_ = menu_key_down;

    const bool undo_key_down = CheckHitKey(KEY_INPUT_U) != 0;
    if (undo_key_down && !undo_key_was_down_) {
        result.undo_requested = true;
    }
    undo_key_was_down_ = undo_key_down;

    // View rotation: arrow keys and right-button drag. Horizontal direction
    // is inverted relative to the naive mapping (drag right -> view turns left).
    if (CheckHitKey(KEY_INPUT_LEFT)) yaw_ += kKeyRotateStep;
    if (CheckHitKey(KEY_INPUT_RIGHT)) yaw_ -= kKeyRotateStep;
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
        yaw_ -= static_cast<float>(mx - last_mouse_x_) * kDragRotatePerPixel;
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
