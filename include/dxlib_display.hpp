#pragma once

#include <dxlib.h>
#include "board.hpp"

namespace cubo_othello {

class DXLibDisplay {
public:
    // Init window & DXLib environment
    bool init(int width = 120, int height = 80);   // returns false on failure

    // Full frame render loop body (called each tick)
    void render_frame(const CubeBoard& board);

    // Render cube wireframe faces (pseudo-3D wireframe)
    void draw_cube_faces();

    // Draw internal grid lines for each visible face
    void draw_grid();

    // Render all 512 stone ellipses with depth-based lighting
    void draw_stones(const CubeBoard& board);

    // Overlay valid-move "+" markers (both players simultaneously)
    void mark_valid_moves(const CubeBoard& board, int color);

    // Process key/mouse events; returns true if frame should continue
    bool handle_input();

private:
    int m_width{} = 120;   // window width
    int m_height{} = 80;   // window height (height in pixels)
};

} // namespace cubo_othello
