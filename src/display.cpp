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

// Stone core fills (kept as arrays so a flipping stone can lerp between them).
constexpr int kBlackCore[3] = {12, 18, 40};
constexpr int kWhiteCore[3] = {255, 235, 255};

// --- move-animation timings (ms) ---
constexpr long long kPlaceAnimMs = 260; // placed-stone pop-in with overshoot
constexpr long long kFlipAnimMs = 430;  // flipped-stone color transition
constexpr long long kShakeMs = 280;     // screen-shake decay

float clamp01(float t) { return t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t); }

// Overshoot easing for the placed-stone pop (grows past full size, settles).
float ease_out_back(float t) {
    const float c1 = 1.70158f, c3 = c1 + 1.0f;
    const float u = t - 1.0f;
    return 1.0f + c3 * u * u * u + c1 * u * u;
}

// Linear blend of two palette colors, then shaded by `light` like rgb().
unsigned int mix_color(const int (&a)[3], const int (&b)[3], float t, float light) {
    const float u = clamp01(t);
    return GetColor(shade(static_cast<int>(a[0] + (b[0] - a[0]) * u), light),
                    shade(static_cast<int>(a[1] + (b[1] - a[1]) * u), light),
                    shade(static_cast<int>(a[2] + (b[2] - a[2]) * u), light));
}

// Deterministic hash for the twinkling starfield positions.
unsigned int star_hash(unsigned int i) {
    i *= 2654435761u;
    i ^= i >> 15;
    i *= 2246822519u;
    i ^= i >> 13;
    return i;
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

    // Twinkling starfield: deterministic positions, sine-wave alpha.
    const long long now = GetNowCount();
    for (int i = 0; i < 70; ++i) {
        const unsigned int hsh = star_hash(static_cast<unsigned int>(i) + 1u);
        const int x = static_cast<int>(hsh % static_cast<unsigned int>(w));
        const int y = static_cast<int>((hsh >> 16) % static_cast<unsigned int>(h));
        const float tw = 0.5f + 0.5f * std::sin(static_cast<float>(now) * 0.001f + i * 0.7f);
        SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(30.0f + 120.0f * tw));
        const unsigned int col = (i % 5 == 0) ? GetColor(160, 220, 255)
                               : (i % 7 == 0) ? GetColor(255, 170, 230)
                                              : GetColor(140, 160, 210);
        DrawCircle(x, y, (i % 9 == 0) ? 2 : 1, col, TRUE);
    }

    // Slow CRT band sweeping downward for a live-screen feel.
    const int band = static_cast<int>(now * 0.05) % (h + 140) - 70;
    for (int dy = 0; dy < 4; ++dy) {
        SetDrawBlendMode(DX_BLENDMODE_ADD, 22 - 5 * dy);
        DrawLine(0, band + dy, w, band + dy, GetColor(90, 170, 230));
    }
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
    // Decaying screen shake (set by render_frame right after a move).
    sx += shake_x_;
    sy += shake_y_;
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

    // Is a move animation currently playing? (render_frame expires move_fx_.)
    const long long now = GetNowCount();
    bool fx_active = false;
    long long fx_age = 0;
    if (move_fx_.has_value()) {
        fx_age = now - move_fx_->t0_ms;
        fx_active = fx_age < kFlipAnimMs + 120;
    }

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

        bool is_placed = false, is_flipped = false;
        float ft = 0.0f, pt = 1.0f;
        if (fx_active) {
            const auto& fx = *move_fx_;
            is_placed = (c.x == fx.placed.x && c.y == fx.placed.y && c.z == fx.placed.z);
            for (const auto& f : fx.flipped) {
                if (f.x == c.x && f.y == c.y && f.z == c.z) { is_flipped = true; break; }
            }
            ft = clamp01(static_cast<float>(fx_age) / static_cast<float>(kFlipAnimMs));
            pt = clamp01(static_cast<float>(fx_age) / static_cast<float>(kPlaceAnimMs));
        }

        // Spawn impact sparks once, at the placed cell's projected position.
        if (is_placed && !move_fx_->sparks_spawned) {
            spawn_sparks(sxf, syf, scale, move_fx_->color);
            move_fx_->sparks_spawned = true;
        }

        // Placed stone pops in with an overshoot, then settles to full size.
        int radius = r;
        if (is_placed) {
            radius = std::max(2, static_cast<int>(r * std::max(0.15f, ease_out_back(pt))));
        }

        // BLACK = dark core with cyan neon; WHITE = bright core with magenta neon.
        const bool is_black = (cell == BLACK);
        const auto& target_neon = is_black ? kCyan : kMagenta;
        int neon[3];
        if (is_flipped) {
            // Ring color transitions from the opponent's neon to ours.
            const auto& old_neon = (move_fx_->color == BLACK) ? kMagenta : kCyan;
            for (int i = 0; i < 3; ++i) {
                neon[i] = static_cast<int>(old_neon[i] + (target_neon[i] - old_neon[i]) * ft);
            }
        } else {
            for (int i = 0; i < 3; ++i) neon[i] = target_neon[i];
        }

        // Ambient neon breathing, phase-shifted per stone.
        const float breathe = 0.5f + 0.5f * std::sin(static_cast<float>(now) * 0.003f + (c.x + c.y + c.z));
        glow(sx, sy, radius, neon, static_cast<int>((32.0f + 14.0f * breathe) * light));
        if (is_flipped) {
            // Core color lerps from the opponent's core to ours, with a white flash on top.
            const auto& old_core = (move_fx_->color == BLACK) ? kWhiteCore : kBlackCore;
            const auto& new_core = is_black ? kBlackCore : kWhiteCore;
            DrawCircle(sx, sy, radius, mix_color(old_core, new_core, ft, light), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(170.0f * (1.0f - ft)));
            DrawCircle(sx, sy, radius, GetColor(255, 245, 255), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        } else if (is_black) {
            DrawCircle(sx, sy, radius, GetColor(shade(12, light), shade(18, light), shade(40, light)), TRUE);
        } else {
            DrawCircle(sx, sy, radius, GetColor(shade(255, light), shade(235, light), shade(255, light)), TRUE);
        }
        DrawCircle(sx, sy, radius, rgb(neon, light), FALSE, 2);
        // Specular highlight so the stone reads as a sphere.
        DrawCircle(sx - radius / 3, sy - radius / 3, std::max(1, radius / 5),
                   is_black ? rgb(kCyan, 0.7f * light) : GetColor(255, 255, 255), TRUE);

        // Expanding shockwave ring around a just-placed stone.
        if (is_placed && pt < 1.0f) {
            const int rr = radius + static_cast<int>(70.0f * pt * scale);
            SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(150.0f * (1.0f - pt)));
            DrawCircle(sx, sy, rr, rgb(neon, 0.6f), FALSE, 3);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        }
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
    const long long now = GetNowCount();

    // Advance the move animation lifecycle and decay the screen shake.
    if (move_fx_.has_value()) {
        const long long age = now - move_fx_->t0_ms;
        if (age > kFlipAnimMs + 400) {
            move_fx_.reset();
            shake_x_ = shake_y_ = 0.0f;
        } else if (age < kShakeMs) {
            const float a = 1.0f - static_cast<float>(age) / static_cast<float>(kShakeMs);
            shake_x_ = 5.0f * a * std::sin(static_cast<float>(age) * 0.09f);
            shake_y_ = 3.0f * a * std::cos(static_cast<float>(age) * 0.13f);
        } else {
            shake_x_ = shake_y_ = 0.0f;
        }
    }

    ClearDrawScreen();
    draw_background(width_, height_);

    draw_cube_faces();
    draw_grid();
    draw_stones(board);
    mark_valid_moves(board, turn);
    draw_sparks();

    // --- HUD -------------------------------------------------------------
    text_glow(20, 14, "キューブ・オセロ", kCyan, 28);
    DrawLine(20, 50, 300, 50, rgb(kCyan, 0.6f));

    const bool black_turn = (turn == BLACK);
    char buf[128];
    const float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(now) * 0.004f);

    // Pulsing stone chip shows whose turn it is.
    glow(34, 72, 12, black_turn ? kCyan : kMagenta, static_cast<int>(90 + 90 * pulse));
    DrawCircle(34, 72, 12, rgb(black_turn ? kBlackCore : kWhiteCore, 1.15f), TRUE);
    DrawCircle(34, 72, 12, rgb(black_turn ? kCyan : kMagenta), FALSE, 2);

    std::string who;
    bool thinking = false;
    if (spectate_) {
        who = "  ＜AI同士で観戦中";
        thinking = true;
    } else if (ai_color_) {
        if (*ai_color_ == turn) {
            who = "  ＜AIの番";
            thinking = true;
        } else {
            who = "  ＜あなたの番＞";
        }
    }
    if (thinking) {
        const int dots = 1 + static_cast<int>((now / 400) % 3); // animated "thinking" dots
        for (int i = 0; i < dots; ++i) who += "・";
        who += "＞";
    }
    std::snprintf(buf, sizeof buf, "手番： %s%s", black_turn ? "黒（シアン）" : "白（マゼンタ）", who.c_str());
    text_glow(54, 60, buf, black_turn ? kCyan : kMagenta, 22);

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
    // Slide the divider smoothly toward the live ratio instead of snapping.
    const float target = static_cast<float>(b) / static_cast<float>(total);
    if (score_blend_ < 0.0f) {
        score_blend_ = target;
    } else {
        const double dt = std::min(100.0, static_cast<double>(now - last_frame_ms_));
        score_blend_ += (target - score_blend_) * static_cast<float>(dt / 240.0);
    }
    const int b_px = static_cast<int>(bar_w * score_blend_);
    DrawString(bar_x, 14, "石の数", color_dim_text());
    DrawBox(bar_x - 2, bar_y - 2, bar_x + bar_w + 2, bar_y + 16, GetColor(10, 10, 30), TRUE);
    DrawBox(bar_x, bar_y, bar_x + b_px, bar_y + 14, rgb(kCyan, 0.85f), TRUE);
    DrawBox(bar_x + b_px, bar_y, bar_x + bar_w, bar_y + 14, rgb(kMagenta, 0.85f), TRUE);
    DrawBox(bar_x - 2, bar_y - 2, bar_x + bar_w + 2, bar_y + 16, rgb(kCyan, 0.7f), FALSE);
    // Glowing divider at the boundary of the two shares.
    SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(70 + 70 * pulse));
    DrawLine(bar_x + b_px, bar_y - 4, bar_x + b_px, bar_y + 18, GetColor(255, 255, 255), 2);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
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
        const int tx = width_ / 2 - tw / 2, ty = height_ / 2 - 20;
        // Dark panel behind so the result reads clearly over the board.
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 180);
        DrawBox(tx - 26, ty - 16, tx + tw + 26, ty + 48, GetColor(8, 5, 22), TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        // Pulse the text color between yellow and white.
        const float mp = 0.5f + 0.5f * std::sin(static_cast<float>(now) * 0.003f);
        const int col[3] = {255, 240 + static_cast<int>(15 * mp), 60 + static_cast<int>(195 * mp)};
        text_glow(tx, ty, status_message.c_str(), col, 32);
    }

    last_frame_ms_ = now;
    ScreenFlip();
}

void DXLibDisplay::notify_move(const Move& placed, const std::vector<Move>& flipped, int8_t color) {
    MoveFx fx;
    fx.placed = placed;
    fx.flipped = flipped;
    fx.color = color;
    fx.t0_ms = GetNowCount();
    move_fx_ = fx;
}

void DXLibDisplay::clear_effects() {
    move_fx_.reset();
    sparks_.clear();
    shake_x_ = shake_y_ = 0.0f;
    score_blend_ = -1.0f;
}

void DXLibDisplay::reset_view() {
    yaw_ = kDefaultYaw;
    pitch_ = kDefaultPitch;
}

void DXLibDisplay::spawn_sparks(float x, float y, float scale, int8_t color) {
    const long long now = GetNowCount();
    // Drop expired sparks so the list stays short.
    sparks_.erase(std::remove_if(sparks_.begin(), sparks_.end(),
                                [now](const Spark& s) { return now - s.t0_ms > s.life_ms; }),
                  sparks_.end());
    std::uniform_real_distribution<float> angle(0.0f, 2.0f * kPi);
    std::uniform_real_distribution<float> speed(30.0f, 170.0f * scale);
    std::uniform_int_distribution<int> life(350, 700);
    const auto& base = (color == BLACK) ? kCyan : kMagenta;
    for (int i = 0; i < 14; ++i) {
        Spark s;
        const float a = angle(fx_rng_), v = speed(fx_rng_);
        s.x = x;
        s.y = y;
        s.vx = std::cos(a) * v;
        s.vy = std::sin(a) * v * 0.7f - 40.0f; // slight upward bias
        s.t0_ms = now;
        s.life_ms = life(fx_rng_);
        // Some sparks flash yellow-hot for a metallic impact look.
        const float mixt = (i % 3 == 0) ? 1.0f : 0.35f;
        s.r = base[0] + static_cast<int>((kYellow[0] - base[0]) * mixt);
        s.g = base[1] + static_cast<int>((kYellow[1] - base[1]) * mixt);
        s.b = base[2] + static_cast<int>((kYellow[2] - base[2]) * mixt);
        sparks_.push_back(s);
    }
}

void DXLibDisplay::draw_sparks() {
    if (sparks_.empty()) return;
    const long long now = GetNowCount();
    for (const Spark& s : sparks_) {
        const long long age = now - s.t0_ms;
        if (age > s.life_ms) continue;
        const float t = static_cast<float>(age) / static_cast<float>(s.life_ms);
        const int px = static_cast<int>(s.x + s.vx * t);
        const int py = static_cast<int>(s.y + s.vy * t + 120.0f * t * t); // gravity
        SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(220.0f * (1.0f - t)));
        DrawCircle(px, py, std::max(1, static_cast<int>(3.0f * (1.0f - t))), GetColor(s.r, s.g, s.b), TRUE);
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
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

    // Opening-position demo board slowly rotating behind the menu panels.
    yaw_ += 0.006f;
    pitch_ = kDefaultPitch + 0.12f * std::sin(static_cast<float>(GetNowCount()) * 0.0008f);
    {
        CubeBoard demo;
        demo.initialize();
        draw_cube_faces();
        draw_grid();
        draw_stones(demo);
    }

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
