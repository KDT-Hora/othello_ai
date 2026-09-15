---
name: Cube Othello Specification
version: 0.2.0
author: koyo
status: refined
speckit-version: 1.0
---

# Cube Othello — Game Specification (Refined)

## Overview

Cube Othello is a 3D cube variant of the classic Reversi board game, played on an **8×8×3** grid (512 cells total). Two players, Black and White, take turns placing stones. The objective is to capture more stones than your opponent by sandwiching their pieces between two of your own along the six cardinal axes (+x, -x, +y, -y, +z, -z).

## Technical Stack

- **Language:** C++17
- **Graphics Library:** DXLib (Windows only)
- **Testing Framework:** Google Test (`gtest`)
- **Build System:** CMake 3.20+
- **Target Platform:** Windows x64 (Visual Studio 2022 / MinGW-w64)

## Game Board

### Shape & Coordinates

The board is a cube of size 8×8×3 with axes defined as:

| Axis | Direction   | Range |
|------|--------------|-------|
| x    | Left → Right | 0–7   |
| y    | Top → Bottom | 0–2   |
| z    | Front → Back| 0–2   |

### Cell States

- `.` (empty) — a valid placement cell
- `B` (Black stone) — player 0's piece
- `W` (White stone) — player 1's piece

### Initial Configuration

Stones are placed in the center region of the cube, at coordinates `(x,y,z)` where `x ∈ {3,4}` and `(y,z)` is any combination from `{(0,0), (0,2), (1,0), (1,2)}`. This gives:

| x | y | z | Player |
|---|---|---|--------|
| 3 | 0 | 0 | B      |
| 4 | 0 | 0 | W      |
| 3 | 2 | 0 | B      |
| 4 | 2 | 0 | W      |
| 3 | 0 | 2 | B      |
| 4 | 0 | 2 | W      |
| 3 | 2 | 2 | B      |
| 4 | 2 | 2 | W      |

This gives an **8-stone initial configuration** (4 Black, 4 White).

## Game Rules

### Turn Order

- **Black starts first.** Each turn, the current player must place one stone on an empty cell where a valid sandwich move exists.
- If no legal moves exist for either player, the game ends in a draw (or via pass rules).

### Sandwich Detection (6 Directions Only)

After placing a stone at `(x,y,z)` with color `C`, check each of the six axes:

```
      +z                    -z
   ←(-x)→     (−z)← →(+x)→    (+z)→
      ↑                ↓
   −y            +y
```

For each direction, scan outward from `(x,y,z)` until hitting either:
- A stone of the opponent's color — if the next cell beyond it is a same-color stone or the board edge, the sandwich succeeds.
- An empty cell or own stone — no sandwich in this direction.

The number of flipped stones equals the total count of all successful sandwiches across all six directions.

### Turn Passing (Pass Rule)

If a player has **no legal moves** on their turn, they pass. The turn passes to the other player. If both players pass consecutively, the game ends.

### Game End Conditions

The game ends when either:
1. **Board full:** No empty cells remain.
2. **No moves available:** Both players have zero legal moves on consecutive turns.

### Victory Condition

Count the number of Black and White stones on the board:
- If `Black > White` → Black wins
- If `White > Black` → White wins
- If equal → Draw

## GUI / Rendering (DXLib Display Layer)

The rendering is handled by a separate **display layer** that communicates with the game engine via callbacks. Key design decisions:

### Projection Model — Flat Plane Isometric View

The 3D cube is projected onto a 2D plane using an oblique projection:

```
int project_x(int x, int y) { return static_cast<int>(x * 0.5f - y * 0.268f); }
int project_y(int y, int z)    { return static_cast<int>(y * 0.5f + z * 0.268f); }
float get_ambient_light(int z) { return std::max(0.3f, 1.0f - static_cast<float>(z)/7.0f * 0.5f); }
```

- The x-axis stretches horizontally (factor ~0.5).
- The y-z plane is foreshortened with a 26.8° shear angle.
- Depth `z` modulates ambient light: front faces appear brighter, back faces darker — giving a sense of depth without true occlusion.

### Rendering Layers

Each frame renders in this order:
1. Background (dark gray)
2. Cube wireframe (8 face borders)
3. Grid lines (thin, lighter gray)
4. Stones (ellipses with shading based on `z`)
5. Valid move markers ("+") on valid cells
6. Turn indicator / game status text

### Input Handling

- **Mouse click:** Places a stone at the clicked cell if it is a legal move for the current player.
- **ESC key:** Ends the application.
- **'R' key or double-click:** Resets the board to initial state.

## DXLib Wrapper Interface

The `DXLibDisplay` class wraps DXLib calls and exposes the following API:

| Method | Signature | Description |
|--------|-----------|-------------|
| `init()` | `void init(int width = 120, int height = 80)` | Initialize DXLib and create window |
| `render_frame()` | `void render_frame() const` | Render a single frame |
| `draw_cube_faces()` | `void draw_cube_faces() const` | Draw the cube wireframe |
| `draw_grid()` | `void draw_grid() const` | Draw grid lines |
| `draw_stones(const CubeBoard&)` | `void draw_stones(...) const` | Render all layers of stones |
| `mark_valid_moves(const CubeBoard&, int color)` | `void mark_valid_moves(...) const` | Overlay "+" markers on valid moves |
| `handle_input()` | `bool handle_input()` | Process keyboard and mouse events |
| `is_game_running()` | `bool is_game_running() const` | Check if the game loop should continue |

## AI Specification (SimpleAI)

The AI uses a **depth-limited minimax** algorithm with evaluation function:

```cpp
int evaluate(const CubeBoard& board, int color) {
  return count_pieces(board, color) - count_pieces(board, other(color));
}
```

- **Search depth:** Default `3` plies (one full turn = black + white = 2 plies).
- **No alpha-beta pruning** in the base implementation — plain minimax.
- **Randomization** is used to break ties among equally-valued moves.

> This AI is intentionally weak for human playability. Advanced evaluation functions (mobility, edge control, etc.) are out of scope and can be added as an optional extension.

## Build & Testing

### CMake Setup

```bash
mkdir build && cd build
cmake .. -DCMAKE_CXX_COMPILER=g++          # or cl.exe for MSVC
cmake --build . --config Release -j4

ctest        # Run unit tests
ctest --output-on-failure   # Verbose output
```

### Coverage Measurement (gcov)

```bash
cmake .. -DCMAKE_CXX_FLAGS="-fprofile-arcs -ftest-coverage" -G "Visual Studio 17 2022"
cmake --build . --config Release
gcov src/game_engine/board.cpp tests/unittests/board_tests.cpp > board_coverage.txt
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_report
```

### Target Coverage Goal: ≥90%

## References

- [Cube Othello Game Rules (Wikipedia)](https://en.wikipedia.org/wiki/Othello_(board_game))
- [DXLib Library](https://www.fmarcelino.com/DXLib/)
- [C++17 Standard Library Reference](https://en.cppreference.com/w/cpp)

---

*This document is part of the Cube Othello project documentation. It defines the complete game rules, rendering behavior, and AI behavior for a C++ + DXLib implementation.*
