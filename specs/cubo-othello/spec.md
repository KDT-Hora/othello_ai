---
name: "Cube Othello - C++ DXLib Implementation"
description: |
  Cube Othello (8×8×3 cubic board) implemented in C++ with DXLib rendering and TDD-driven development.
  The game features a flat-plane isometric projection display, simple minimax AI evaluated by stone count,
  and comprehensive unit + E2E test coverage targeting ≥90%.

short-name: cubo-othello-cpp-dxlib
---

# Cube Othello — C++/DXLib Specification 🎲

## Overview

Cube Othello is a 3D extension of the classic Othello (Reversi) game played on an **8×8×8 cube** (512 cells).
This specification defines the C++ implementation using **DXLib** for rendering and **gtest** for testing.

---

## User Scenarios & Testing

### Scenario 1: Human vs. Human Play

> A user launches the game, plays as Black against White on a flat-plane isometric projection view.

Acceptance Criteria:
- Game starts with center pieces placed at `(x,y,z) ∈ {3,4}×{3,4}×{3,5}` (8 stones total).
- The current turn indicator ("BLACK" / "WHITE") updates after each move.
- Valid moves are marked on the board with a "+" symbol.
- A stone placed at `(x,y,z)` flips all opponent stones along the 6 axial directions (+/-x, +/-y, +/-z) where a sandwich is formed.
- If no valid moves exist for either player, the game ends (no-moves condition).
- The winner is determined by comparing final stone counts; if tied, it's a draw.
- Pressing `ESC` exits the game loop.

### Scenario 2: Human vs. AI Play

> A user launches the game and plays against a simple minimax AI that evaluates board state solely by stone count difference (black − white). The AI searches to depth 3 with no pruning.

Acceptance Criteria:
- The AI always passes when it has no valid moves.
- When multiple moves yield equal evaluation, the AI picks one randomly among them.
- The AI never makes an illegal move.

### Scenario 3: Game Over Conditions

> A user wants to know how and when the game ends.

Acceptance Criteria:
- The game ends if **all** cells are filled → draw.
- The game ends if both players have no legal moves → declare winner by stone count (or draw if tied).

### Scenario 4: Performance & Resource Usage

> A user wants to know the performance characteristics of the application.

Acceptance Criteria:
- The game runs smoothly at ≥60 FPS on a standard desktop PC.
- Memory usage stays below ~50 MB during gameplay.
- The application exits cleanly (no dangling DXLib resources) when closed via `ESC` or process termination.

---

## Functional Requirements

### FR-001: Board Initialization

**Description:** Initialize the 8×8×8 cube board with center pieces and set game state to "in-progress."

Acceptance Criteria:
- The board is represented as a 3D array `grid[8][8][8]` where each cell holds `-1` (empty), `0` (Black), or `+1` (White).
- Center stones are placed at coordinates `(x,y,z)` with `x∈{3,4}`, `y∈{3,4}`, `z∈{3,5}`:
  - Black at `(3,3,3)`, `(3,3,4)`, `(3,4,3)`, `(3,4,4)`
  - White at `(4,3,3)`, `(4,3,4)`, `(4,4,3)`, `(4,4,4)`
- `turn` is set to `BLACK`.
- `game_over` flag is false.

### FR-002: Placing a Stone

**Description:** Place a stone of the given color at the specified cell and flip all sandwiched opponent stones along each axis direction.

Acceptance Criteria:
- The move must be on an empty cell; otherwise return error.
- For each of the 6 directions (`+x`, `-x`, `+y`, `-y`, `+z`, `-z`), scan from the immediate neighbor outward:
  - If a sequence of opponent stones is found followed by one's own color, flip all those opponent stones and record their positions.
- After flipping, update the board state with the new stone and flipped stones (all become the placing player's color).
- Return the count of flipped stones; if zero, this move is invalid under Othello rules.

### FR-003: Valid Move Detection

**Description:** Identify all empty cells from which a legal move can be made (i.e., sandwiches opponent stones in at least one direction).

Acceptance Criteria:
- Return a list of `(x,y,z)` tuples representing valid moves for the given player.
- A cell is valid only if placing a stone there results in ≥1 flip along some axis.
- The move count must be ≤512 (full board).

### FR-004: Turn Switching

**Description:** Advance turn from current player to opponent after a successful placement and flip operation.

Acceptance Criteria:
- If the placing player has no valid moves, they pass their turn; the game ends if both players lack legal moves.
- Otherwise, switch `turn` from `BLACK` → `WHITE` or vice versa.

### FR-005: Game Over Detection

**Description:** Determine whether the game should terminate and why.

Acceptance Criteria:
- Return `GAME_OVER_FULL` if all 512 cells are occupied.
- Return `GAME_OVER_NO_MOVES` if both players have zero legal moves.
- Otherwise return `GAME_IN_PROGRESS`.

### FR-006: Stone Count & Result Evaluation

**Description:** Compute final stone counts and determine the winner or draw.

Acceptance Criteria:
- Black wins if black stones > white stones at game end.
- White wins if white stones > black stones.
- Draw if both counts are equal (or if the board is full with no decisive outcome).

### FR-007: Rendering — Flat-Plane Isometric Projection

**Description:** Render the cube board using DXLib in a 2D flat-plane view that visually approximates an isometric projection, without true depth layering or occlusion.

Acceptance Criteria:
- The window size defaults to `120×80×480` pixels (width × height × depth buffer) but can be configured via DXLib init parameters.
- Each cell at `(x,y)` is drawn as a small ellipse/circle with color determined by its content (`.` / `B` / `W`).
- The z-axis (depth) is represented only by **ambient lighting variation**: front-facing cells are brighter, rear cells darker.
  - Lighting formula: `light = max(0.3f, 1.0f - (double)z / 7.0 * 0.5f)` — front (`z=0`) is brightest (~1.0), back (`z=7`) is darkest (~0.29).
- The grid lines are drawn to delineate the three visible cube faces, giving a pseudo-3D appearance without true perspective distortion.
- A "+" marker is drawn above valid move cells (for both players).

### FR-008: Rendering — Stone Appearance

**Description:** Draw each stone as an ellipse with shading appropriate for its color and depth position.

Acceptance Criteria:
- Black stones are rendered as dark gray ellipses (`#3a4a5e` fill, darker stroke).
- White stones are rendered as light gray to off-white ellipses (`#f0f0f5` fill, lighter stroke).
- An optional inner shadow or highlight can be added using elliptical fills with slightly shifted centers.

### FR-009: Rendering — Input Handling

**Description:** Capture user input (keyboard and mouse) for gameplay interaction.

Acceptance Criteria:
- Pressing `ESC` terminates the game loop cleanly.
- Mouse clicks on a valid move cell trigger that player's turn.
- Pressing `R` resets the board to initial state without ending the game.

---

## Non-Functional Requirements

### NFR-001: Performance

**Description:** The application must maintain smooth frame rates and responsive input handling.

Acceptance Criteria:
- Render loop targets ≥60 FPS on a standard desktop PC (Windows 7/8/10).
- Move validation completes within ≤5 ms per candidate cell for the first pass; full board scan is bounded by O(N) where N=512 → worst-case ~1.5 μs, trivially met.

### NFR-002: Build & Compilation

**Description:** The project builds cleanly with no warnings using Visual Studio 2022 (or MinGW-w64 g++).

Acceptance Criteria:
- `cmake .. -G "Visual Studio 17 2022"` followed by `cmake --build . --config Release` completes without errors.
- All tests pass via `ctest`.

### NFR-003: Unit Test Coverage

**Description:** Achieve ≥90% branch/line coverage over game engine logic using gtest + gcov/lcov.

Acceptance Criteria:
- Every public method of `CubeBoard` has at least one test case covering all branches (success, failure paths).
- The same applies to `SimpleAI`'s evaluate and search methods.
- Coverage report (`coverage_report/index.html`) shows ≥90% coverage for the game engine module.

---

## Data Model

### Entity: `BoardCell`

| Field | Type    | Description                         |
|-------|---------|-------------------------------------|
| x     | int32   | Column index (0..7)                |
| y     | int32   | Row index (0..7)                   |
| z     | int32   | Depth layer (0..7), front→back     |
| color | int8    | -1 = empty, 0 = Black, +1 = White  |

### Entity: `GameState`

| Field        | Type | Description                          |
|--------------|------|--------------------------------------|
| turn         | int8 | Current player (-1=unstarted, 0=B, 1=W) |
| game_over    | bool | Whether the game has terminated     |
| result_code  | int  | 0=in_progress, 1=BLACK_WINS, 2=DRAW, 3=NO_MOVES |

### Entity: `DXLibDisplay` (rendering layer)

| Method                  | Description                                                    |
|-------------------------|-----------------------------------------------------------------|
| init(width, height)     | Initialize DXLib window and render context                     |
| render_frame()         | Render one frame: background → grid faces → stones → markers  |
| draw_cube_faces()      | Draw the wireframe of the three visible cube faces             |
| draw_grid()            | Draw internal grid lines                                        |
| draw_stones(board)     | Draw all 512 stone ellipses with depth-based lighting          |
| mark_valid_moves(...)  | Overlay "+" markers on valid move cells                        |
| handle_input()         | Process key/mouse events; return true if frame should continue |

---

## Constraints & Assumptions

### Assumptions

- The target platform is Windows (DXLib). DXLib cross-compilation to Linux/macOS is not in scope.
- No network or external services are involved; the game runs entirely locally.
- DXLib version ≥2.0 is assumed available on the system.

### Constraints

- **No** use of OpenGL, DirectX, SFML, SDL, etc. — only DXLib's 2D graphics API.
- The isometric projection remains a **flat-plane approximation**; no true z-buffer occlusion is used for performance simplicity.
- AI evaluation uses **only stone count difference**; no lookahead beyond depth=3 and no pruning is required (minimax without alpha-beta is acceptable).

---

## Glossary

| Term       | Meaning                                                    |
|------------|-------------------------------------------------------------|
| Cube Othello | A 3D extension of Othello played on an 8×8×8 cube          |
| Sandwich   | Placing a stone such that opponent pieces are "sandwiched" between your own on one or more axial lines |
| Flip       | Convert all sandwiched opponent stones to your color        |
| Flat-Plane Isometric Projection | A pseudo-3D view rendered via 2D projections with z-based lighting, without true occlusion |

---

## Appendix: Algorithmic Details

### Sandwich Detection (per direction)

```text
For each of the six directions (+x,-x,+y,-y,+z,-z):
  current = neighbor in that direction
  while current is within bounds:
    if current == opponent_color → extend sequence
    else if current == my_color and sequence length ≥ 1 → record sandwich, break loop
    else if current == empty or same color as me already placed → stop scanning this direction
```

### Isometric Projection Mapping (flat-plane approximation)

```text
projected_x = x * 0.5 - y * 0.268   // maps to horizontal screen coordinate
projected_y = y * 0.5 + z * 0.268   // maps to vertical screen coordinate
lighting    = max(0.3f, 1.0f - (double)z / 7.0 * 0.5f)
```

### Evaluation Function (for AI)

```text
score(board) = count(black_stones) − count(white_stones)
```

The minimax search uses this score at each leaf node; the player with the higher score is preferred. Depth is capped at 3 plies to maintain responsiveness.

---

## Success Criteria (Summary)

1. ✅ The game runs end-to-end: initialization → placement & flipping → turn switching → game over detection → winner announcement.
2. ✅ The flat-plane isometric view renders smoothly with proper stone shading and valid-move markers.
3. ✅ The AI plays reasonably well within depth-3 minimax, never making illegal moves.
4. ✅ Unit tests cover ≥90% of the codebase; E2E scenarios pass for both human-vs-human and human-vs-AI modes.
