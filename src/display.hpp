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
    bool undo_requested = false;       // 'U' pressed
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
    bool undo_key_was_down_ = false; // edge-detect so holding 'U' doesn't unwind all history at once
    bool left_was_down_ = false;     // edge-detect so one click places exactly one stone
    bool right_was_down_ = false;    // right-drag rotates the view
    int last_mouse_x_ = 0;
    int last_mouse_y_ = 0;
    float yaw_ = 0.6f;               // view rotation about the screen-vertical axis (radians)
    float pitch_ = 0.5f;             // view rotation about the screen-horizontal axis (radians)

    // Rotates board-space point (px,py,pz) (cell centers at integers 0..7)
    // by the current view and perspective-projects it. `depth` is larger
    // for points farther from the viewer; `scale` is the perspective factor.
    void project(float px, float py, float pz, float& sx, float& sy, float& depth, float& scale) const;

    // Nearest-to-viewer legal cell under the mouse pointer, if any.
    std::optional<Move> pick_move(const CubeBoard& board, int8_t turn, int mx, int my) const;
};

} // namespace cubo_othello
