// Cube Othello -- DXLibDisplay: flat-plane isometric rendering + input (DXLib).
#pragma once

#include <optional>
#include <string>

#include "board.hpp"

namespace cubo_othello {

// Result of one input-handling pass.
struct InputResult {
    bool quit = false;                 // ESC pressed, or window closed
    bool reset_requested = false;      // 'R' pressed
    std::optional<Move> clicked_move;  // a legal cell the mouse clicked on
};

class DXLibDisplay {
public:
    bool init(int width = 120, int height = 80);
    void shutdown();

    // Full frame: clear -> wireframe faces -> grid -> stones -> "+" markers
    // for `turn`'s legal moves -> turn/score overlay -> status_message.
    void render_frame(const CubeBoard& board, int8_t turn, const std::string& status_message);

    void draw_cube_faces();
    void draw_grid();
    void draw_stones(const CubeBoard& board);
    void mark_valid_moves(const CubeBoard& board, int8_t color);

    // Processes key/mouse events for the current frame.
    InputResult handle_input(const CubeBoard& board, int8_t turn);

private:
    int width_ = 120;
    int height_ = 80;

    // Project cube cell (x,y,z) to screen coordinates using the flat-plane
    // isometric approximation from the spec.
    void project(int x, int y, int z, int& sx, int& sy) const;
    float lighting(int z) const;
};

} // namespace cubo_othello
