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
    bool menu_requested = false;       // 'M' pressed (back to the mode menu)
    std::optional<Move> clicked_move;  // a legal cell the mouse clicked on
};

// Result of one menu input pass. `mode_row` / `difficulty_col` are -1
// unless the mouse clicked that row/button this frame.
struct MenuInput {
    bool quit = false;
    bool confirm = false;   // Enter/Space, or the START button was clicked
    int move = 0;           // -1 / +1: selection moved up / down by keyboard
    int diff_move = 0;      // -1 / +1: difficulty moved left / right by keyboard
    int mode_row = -1;
    int difficulty_col = -1;
};

inline constexpr int kMenuModeCount = 4;
inline constexpr int kMenuDifficultyCount = 3;

class DXLibDisplay {
public:
    bool init(int width = 120, int height = 80);
    void shutdown();

    // Mode-select screen. `mode`: 0 = human vs human, 1 = human(Black) vs AI,
    // 2 = human(White) vs AI, 3 = AI vs AI (spectate). `difficulty`: 0..2 (easy/normal/hard).
    void render_menu(int mode, int difficulty);
    MenuInput handle_menu_input();

    // Which color the AI plays in the current game (nullopt = human vs
    // human); only used to label the turn in the HUD.
    void set_ai_color(std::optional<int8_t> color) { ai_color_ = color; }
    // AI vs AI: the HUD labels the game as "spectating" instead.
    void set_spectate(bool spectate) { spectate_ = spectate; }

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
    std::optional<int8_t> ai_color_;
    bool spectate_ = false;
    int hover_flip_count_ = -1;      // stones the hovered move would flip; -1 = no hover
    bool menu_key_prev_[5] = {};     // up, down, left, right, confirm
    bool menu_click_prev_ = false;
    bool menu_key_was_down_ = false; // 'M' edge detect
    bool left_was_down_ = false;     // edge-detect so one click places exactly one stone
    bool right_was_down_ = false;    // right-drag rotates the view
    int last_mouse_x_ = 0;
    int last_mouse_y_ = 0;
    float yaw_ = 0.6f;               // view rotation about the screen-vertical axis (radians)
    float pitch_ = 0.5f;             // view rotation about the screen-horizontal axis (radians)

    // Rotates board-space point (px,py,pz) (cell centers at integers 0..BOARD_SIZE-1)
    // by the current view and perspective-projects it. `depth` is larger
    // for points farther from the viewer; `scale` is the perspective factor.
    void project(float px, float py, float pz, float& sx, float& sy, float& depth, float& scale) const;

    // Nearest-to-viewer legal cell under the mouse pointer, if any.
    std::optional<Move> pick_move(const CubeBoard& board, int8_t turn, int mx, int my) const;
};

} // namespace cubo_othello
