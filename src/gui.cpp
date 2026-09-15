#include "include/dxlib_display.hpp"
#include <cmath>
#include <algorithm>
#include <tuple>

namespace cubo_othello {

// ─── Rendering constants ────────────────────────────────────────────────────────

constexpr float LIGHTING_CONSTANT = 0.3f;          // floor shadow limit
constexpr int   CELL_SIZE = 8;                      // pixels per cell (small for 120×80)
constexpr float ISOMETRIC_SCALE_X = 0.5f;           // x → projected-x scale
constexpr float ISOMETRIC_SCALE_Y = 0.5f;           // y → projected-y scale
constexpr float ISOMETRIC_SCALE_Z = 0.268f;         // z → depth offset

// ─── DXLibDisplay implementation ────────────────────────────────────────────────

bool DXLibDisplay::init(int width, int height) {
    m_width = width > 0 ? width : 120;
    m_height = height > 0 ? height : 80;

    // Create DXLib window (RGB565 depth buffer for proper z-ordering of grid lines behind stones)
    if (!DXOpen(m_width, m_height, "Cube Othello")) {
        return false;   // failed to create window
    }

    // Set background color: deep slate (#1C1C2A)
    DXSetBackColor(RGB(0x34, 0x34, 0x50));

    return true;
}

void DXLibDisplay::render_frame(const CubeBoard& board) {
    // Clear screen to background color (DXLib redraws to backcolor if nothing drawn yet)
    DXClear();

    draw_cube_faces();   // wireframe cube faces first → behind stones
    draw_grid();         // grid lines on top of wireframe
    draw_stones(board);  // stones drawn last → occlude grid
    mark_valid_moves(board, BLACK);
    mark_valid_moves(board, WHITE);

    // Render turn indicator & score overlay
    DXSetColor(RGB(0x88, 0xBB, 0xFF));   // light blue text
    DXDrawTextA("Turn: ", 2, 4, FONT_BOLD);
    if (board.turn_ == BLACK) {
        DXDrawTextA("BLACK", 16, 4, FONT_NORMAL);
    } else {
        DXDrawTextA("WHITE", 16, 4, FONT_NORMAL);
    }

    // Score overlay (simple text for now; can be enhanced with custom fonts later)
    int black_count = board.count_pieces(BLACK);
    int white_count = board.count_pieces(WHITE);
    DXSetColor(RGB(0xFF, 0xBB, 0x88));   // light coral score text
    char buf[64]{};

    if (black_count == -1) {
        sprintf(buf, "Black: ? | White: %d", white_count);
    } else {
        sprintf(buf, "Black: %d | White: %d", black_count, white_count);
    }
    DXDrawTextA(buf, 40, 56, FONT_NORMAL);

    if (board.game_over()) {
        DXSetColor(RGB(0xFF, 0x88, 0x88));   // red for game-over message
        char msg[64]{};
        sprintf(msg, "Game Over! %s", board.game_over() ? "Draw / No moves" : "");
        DXDrawTextA(msg, 32, 72, FONT_BOLD);
    }

    // Handle input (ESC to quit)
    handle_input();
}

void DXLibDisplay::draw_cube_faces() {
    // Draw wireframe for all three visible faces:
    //   • Front face  (y=0 plane in projected coords)
    //   • Right face  (x=7 plane, drawn offset rightward)
    //   • Top face    (z=0 plane, drawn offset upward)

    constexpr int FACE_WIDTH = 48;   // ~6 cells wide (CELL_SIZE*6 ≈ 48px)
    constexpr int FACE_HEIGHT = 32;  // ~4 cells tall

    // Draw the three visible faces as wireframe rectangles (outline only, no fill)
    const int outline_width = 1;
    const int face_gap = 2;          // gap between faces for visual separation

    // Front face: centered-left at x=0..FACE_WIDTH-1, y=-FACE_HEIGHT/2 .. +FACE_HEIGHT/2-1 (centered vertically)
    const int front_x = m_width / 4;
    const int front_y = -m_height / 4;
    for (int i = 0; i < FACE_WIDTH; ++i) {
        DXSetLineColor(RGB(0x88, 0xAA, 0xCC));   // light cyan wireframe
        DXDrawLine(front_x + i*CELL_SIZE, front_y - FACE_HEIGHT/2, front_x + (i+1)*CELL_SIZE - CELL_SIZE, front_y - FACE_HEIGHT/2);
    }

    // Right face: offset right by (FACE_WIDTH + gap)
    const int right_x = front_x + FACE_WIDTH + face_gap;
    for (int i = 0; i < FACE_WIDTH; ++i) {
        DXSetLineColor(RGB(0x88, 0xAA, 0xCC));
        DXDrawLine(right_x + i*CELL_SIZE, front_y - FACE_HEIGHT/2, right_x + i*CELL_SIZE, front_y + FACE_HEIGHT/2);
    }

    // Top face: offset upward by (FACE_HEIGHT + gap)
    const int top_y = front_y - FACE_HEIGHT - face_gap;
    for (int i = 0; i < FACE_WIDTH; ++i) {
        DXSetLineColor(RGB(0x88, 0xAA, 0xCC));
        DXDrawLine(front_x + i*CELL_SIZE, top_y, front_x + i*CELL_SIZE, top_y + FACE_HEIGHT);
    }
}

void DXLibDisplay::draw_grid() {
    // Draw internal grid lines for the three visible faces.
    // Grid lines run along x=const (vertical), y=const (horizontal) on each face.

    constexpr int GRID_COLOR = RGB(0x55, 0x77, 99);   // medium blue-gray grid line

    const float projected_x_per_cell = ISOMETRIC_SCALE_X;
    const float projected_y_per_cell = ISOMETRIC_SCALE_Y;

    for (int z = 0; z < Z_LAYERS; ++z) {
        for (int y = 0; y < BOARD_SIZE; ++y) {
            // Draw vertical grid lines (x-axis varies, y fixed) on this face layer
            float px = -BOARD_SIZE / 2.0f * projected_x_per_cell + ISOMETRIC_SCALE_X * (float)(x=0);
            for (int x = 0; x < BOARD_SIZE; ++x) {
                // Draw vertical line along x-axis on the current y-layer:
                DXSetLineColor(GRID_COLOR);
                float px_left  = -BOARD_SIZE / 2.0f + ISOMETRIC_SCALE_X * (float)x;
                float py_bottom = -BOARD_SIZE / 2.0f + ISOMETRIC_SCALE_Y * (float)y;
                float py_top    = BOARD_SIZE / 2.0f + ISOMETRIC_SCALE_Y * (float)y;

                // Map x coordinate to projected-x range [-48, +48] (≈ 6 cells × CELL_SIZE)
                float left_px = -BOARD_SIZE / 2.0f * projected_x_per_cell + ISOMETRIC_SCALE_X * (float)x;
                float right_px = -BOARD_SIZE / 2.0f * projected_x_per_cell + ISOMETRIC_SCALE_X * (float)(x+1);

                // For y-axis lines, draw along the entire height of this face at fixed x
                DXDrawLine(left_px, py_bottom, left_px, py_top);
            }
        }
    }
}

void DXLibDisplay::draw_stones(const CubeBoard& board) {
    // Render each cell (x,y,z) as an ellipse with depth-based lighting.
    // Lighting formula: light = max(0.3f, 1.0f - z/7.0*0.5f)

    const float projected_x_per_cell = ISOMETRIC_SCALE_X;
    const float projected_y_per_cell = ISOMETRIC_SCALE_Y;

    for (int z = 0; z < Z_LAYERS; ++z) {
        for (int y = 0; y < BOARD_SIZE; ++y) {
            for (int x = 0; x < BOARD_SIZE; ++x) {
                int color_code = board.grid_[x][y][z];

                // Compute lighting factor: front-facing cells brightest, back-face darkest (~0.29)
                float light_factor = std::max(0.3f, 1.0f - static_cast<float>(z)/7.0f*0.5f);

                int fill_rgb[3]{};
                int stroke_rgb[3]{};

                if (color_code == EMPTY) {
                    // Empty cell: light gray dot
                    fill_rgb = {0xAAAAAA, 0xAAAAAA, 0xAAAAAA};   // ~70% gray
                    stroke_rgb = {0xCCCCCC, 0xCCCCCC, 0xCCCCCC}; // slightly lighter outline
                } else if (color_code == BLACK) {
                    // Black stone: dark slate (#3a4a5e) with darker stroke
                    fill_rgb = {static_cast<int>(0x3a * light_factor), static_cast<int>(0x4a * light_factor), static_cast<int>(0x5e * light_factor)};
                    stroke_rgb = {static_cast<int>(0x2a * (light_factor+0.1f)), static_cast<int>(0x3a * (light_factor+0.1f)), static_cast<int>(0x46 * (light_factor+0.1f))};
                } else { // WHITE
                    // White stone: off-white (#f0f0f5) with lighter stroke
                    fill_rgb = {(int)(240*light_factor), (int)(240*light_factor), (int)(243*light_factor)};
                    stroke_rgb = {(int)(160*light_factor), (int)(160*light_factor), (int)(165*light_factor)};
                }

                // Project cell center to 2D screen coordinates
                float px = -BOARD_SIZE / 2.0f * projected_x_per_cell + ISOMETRIC_SCALE_X * static_cast<float>(x);
                float py = -BOARD_SIZE / 2.0f * projected_y_per_cell + ISOMETRIC_SCALE_Y * static_cast<float>(y)
                        + ISOMETRIC_SCALE_Z * static_cast<float>(z);   // z adds upward offset for pseudo-3D

                // Draw filled ellipse (stone body)
                int r = CELL_SIZE / 2;
                DXSetFillColor(fill_rgb[0], fill_rgb[1], fill_rgb[2]);
                DXFillEllipse(px + static_cast<float>(r), py, static_cast<float>(r));

                // Draw stroke outline
                DXSetLineColor(stroke_rgb[0], stroke_rgb[1], stroke_rgb[2]);
                DXDrawEllipse(px + static_cast<float>(r), py, static_cast<float>(r));

                // Optionally draw inner letter (B/W) — only if needed for clarity
            }
        }
    }
}

void DXLibDisplay::mark_valid_moves(const CubeBoard& board, int color) {
    auto valid = board.get_valid_moves(color);
    const float projected_x_per_cell = ISOMETRIC_SCALE_X;
    const float projected_y_per_cell = ISOMETRIC_SCALE_Y;

    for (const auto& pos : valid) {
        int x = std::get<0>(pos), y = std::get<1>(pos), z = std::get<2>(pos);

        // Project to 2D screen coordinates
        float px = -BOARD_SIZE / 2.0f * projected_x_per_cell + ISOMETRIC_SCALE_X * static_cast<float>(x);
        float py = -BOARD_SIZE / 2.0f * projected_y_per_cell + ISOMETRIC_SCALE_Y * static_cast<float>(y)
                + ISOMETRIC_SCALE_Z * static_cast<float>(z);

        // Draw bright green "+" marker above the cell center
        DXSetTextColor(RGB(0x00, 0xFF, 0x88));   // vivid lime-green
        int font_size = FONT_NORMAL;
        float text_x = px + CELL_SIZE / 2.0f - 1.5f*CELL_SIZE/4.f;   // center horizontally
        float text_y = py - 3.0f * CELL_SIZE / 8.f;                  // slightly above cell

        DXDrawTextA("+", (int)(text_x), (int)text_y, font_size);
    }
}

bool DXLibDisplay::handle_input() {
    int key = DXGetKey();

    if (key == KEY_ESCAPE) {
        DXClose();   // shut down DXLib window cleanly
        return false;   // tell caller: exit game loop
    } else if (key == 'r' || key == 'R') {
        // Reset board to initial state without ending the game
        // Called via GameEngine wrapper later.
    }

    return true;   // continue rendering loop on next tick
}

} // namespace cubo_othello
