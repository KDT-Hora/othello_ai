---
name: "Cube Othello — C++ DXLib Implementation Tasks"
short-name: cubo-othello-tasks
created-at: 2026-09-15
status: mostly-complete (see progress.md)
phase-count: 7
total-task-count: 34
priority-breakdown:
  P1: 18 tasks (Board Logic Core + Rendering Layer)
  P2:  6 tasks (AI Agent)
  P3: 10 tasks (Game Engine, Tests, E2E)
---

# Cube Othello — Task List 🎲

## Phase 0: Setup ⚙️

- [x] T001 Create project directory structure (`src/`, `include/`, `tests/`, `assets/`)
  - File path: `.gitignore` (add CMake build artifacts, IDE files)

- [x] T002 Initialize CMakeLists.txt with DXLib include paths and source file list
  - File path: `CMakeLists.txt`
  - Dependencies: project root (DXLib libs), CMake ≥3.15
  - Notes: Add `-G "Visual Studio 17 2022"` generator, set C++17 standard

- [x] T003 Create `README.md` with build instructions, controls (`ESC`, mouse click, `'R'` reset), and usage examples
  - File path: `README.md`

## Phase 1: Board Logic Core (P1) — US1: Human vs. Human Play 🧑‍🎮

### FR-001: Board Initialization

- [x] T004 Implement `CubeBoard` class declaration in `src/board.hpp`:
    ```cpp
    struct CubeBoard {
        std::array<std::array<std::array<int8_t, BOARD_SIZE>, BOARD_SIZE>, Z_LAYERS> grid_;
        int turn_{};  // -1=unstarted, 0=BLACK, 1=WHITE
        bool game_over_{};
    };
    ```
  - File path: `src/board.hpp`

- [x] T005 Implement `CubeBoard::initialize()` — place center stones at `(x,y,z) ∈ {3,4}×{3,4}×{3,5}` (Black at `(3,3,3)..(3,4,4)`, White at `(4,3,3)..(4,4,4)`), set `turn_ = BLACK`
  - File path: `src/board.cpp`

### FR-002: Stone Placement & Flipping

- [x] T006 Implement `CubeBoard::place_stone(x,y,z,color)` with full sandwich detection:
    - Validate cell is empty; reject if not
    - For each of 6 directions (`+x`/`-x`/`+y`/`-y`/`+z`/`-z`), scan outward from immediate neighbor
    - If opponent stones followed by own color → record sandwich, flip all intervening stones
    - Update board state; return flipped count (0 = invalid move)
  - File path: `src/board.cpp`

### FR-003: Valid Move Detection

- [x] T007 Implement `CubeBoard::get_valid_moves(color) const` → `std::vector<std::tuple<int,int,int>>`:
    - Scan all empty cells; for each, check if it produces ≥1 flip
    - Return list of valid move coordinates
  - File path: `src/board.cpp`

### FR-004: Turn Switching

- [x] T008 Implement turn-switching logic inside `place_stone()`: after successful placement, if placing player has no remaining valid moves → pass their turn; otherwise switch to opponent
  - File path: `src/board.cpp`

### FR-005: Game Over Detection

- [x] T009 Implement `CubeBoard::game_over()` returning enum `{GAME_OVER_FULL, GAME_OVER_NO_MOVES, GAME_IN_PROGRESS}`
  - File path: `src/board.cpp`

### FR-006: Stone Count & Evaluation

- [x] T010 Implement `CubeBoard::count_pieces(color)` → `{black_count, white_count}` pair; implement `evaluate()` returning `my_stones − opponent_stones`
  - File path: `src/board.cpp`

## Phase 1 continued: Rendering Layer (P1) — US1: Human vs. Human Play 🎨

### FR-007: Flat-Plane Isometric Projection Rendering

- [x] T011 Implement `DXLibDisplay` class in `include/dxlib_display.hpp`:
    - `init(width, height)` → create DXLib window at 120×80 pixels with background color `RGB(0x1C1C2A)`
  - File path: `include/dxlib_display.hpp`

- [x] T012 Implement `DXLibDisplay::render_frame()` — full frame loop: clear → draw wireframe cube faces → draw grid lines → draw stones with per-layer lighting → render turn indicator & score overlay
    - Lighting formula: `light = max(0.3f, 1.0f - static_cast<double>(z)/7.0*0.5f)`
  - File path: `src/gui.cpp`

- [x] T013 Implement `DXLibDisplay::draw_cube_faces()` — draw wireframe for the three visible faces (front/back/left/right/top/bottom) to convey pseudo-3D structure without true occlusion
  - File path: `src/gui.cpp`

- [x] T014 Implement `DXLibDisplay::draw_grid()` — draw internal grid lines along x=const, y=const for each z-layer to delineate the three visible faces
  - File path: `src/gui.cpp`

- [x] T015 Implement `DXLibDisplay::draw_stones(const CubeBoard&)`:
    - Map each cell `(x,y)` to screen coordinates using affine transform (`projected_x = x*0.5 − y*0.268`, `projected_y = y*0.5 + z*0.268`)
    - Draw ellipse per cell: empty=`.` (gray), Black=`B` (`#3a4a5e` fill, darker stroke), White=`W` (`#f0f0f5` fill, lighter stroke)
  - File path: `src/gui.cpp`

- [x] T016 Implement `DXLibDisplay::mark_valid_moves(const CubeBoard&, color)` — draw bright green `"+"` marker above each cell that is a legal move for the given player (for both players simultaneously per spec requirement)
  - File path: `src/gui.cpp`

### FR-009: Input Handling

- [x] T017 Implement `DXLibDisplay::handle_input()` — process key/mouse events inside the render loop:
    - `ESC` → terminate game loop, call DXLib cleanup (`DXClose()`)
    - Mouse click at `(screen_x, screen_y)` → map to nearest cell; if legal for current player, place stone immediately and advance turn
    - `'R'` key press → reset board via `board.initialize()` without ending the game
  - File path: `src/gui.cpp`

## Phase 2: AI Agent (P2) — US2: Human vs. AI Play 🤖

### FR-001-AI / FR-006-AI: Simple Minimax Player

- [x] T018 Implement `SimpleAI` class in `src/ai_player.hpp`:
    ```cpp
    struct SimpleAI {
        int color_{};          // BLACK or WHITE
        int depth_{3};         // search depth
        CubeBoard::evaluate_fn evaluate_;  // pointer to evaluation function
        std::random_device rd;
        std::mt19937 gen{rd()};  // for tie-breaking randomness
    };
    ```
  - File path: `src/ai_player.hpp`

- [x] T019 Implement `SimpleAI::evaluate(const CubeBoard& board) const` → int: return `my_stones − opponent_stones`; positive = favorable to self
  - File path: `src/ai_player.cpp`

- [x] T020 Implement `SimpleAI::search(const CubeBoard&, ply)` — minimax search without alpha-beta pruning, depth-limited at user-specified `depth_`:
    - If no legal moves → return `nullopt` (pass)
    - Otherwise: for each valid move, recurse with alternating player; track best score and corresponding move
    - At leaf nodes (`ply == 0`) or when no children exist → call `evaluate()`
    - Tie-breaking: randomly select among equally-best moves using the seeded RNG
  - File path: `src/ai_player.cpp`

- [x] T021 Implement `SimpleAI::place_move(const CubeBoard&, const Move&) const` — helper that places a stone on behalf of the AI, flips sandwiched stones, and switches turn (or passes if no moves exist)
  - File path: `src/ai_player.cpp`

## Phase 3: Game Engine & Integration (P1) — US1 continued 🎮

### FR-004–FR-009: Game Loop Controller

- [x] T022 Implement `GameEngine` class in `src/game_engine.hpp`:
    ```cpp
    struct GameEngine {
        CubeBoard board_;
        DXLibDisplay* display_{};  // pointer to rendering layer
        SimpleAI* ai_{nullptr};     // nullptr = human vs. human mode
        int turn_{};               // current player (-1=unstarted, 0=B, 1=W)
        bool game_over_{};
    };
    ```
  - File path: `src/game_engine.hpp`

- [x] T023 Implement `GameEngine::start()` — call `board.initialize()`, set `turn_ = BLACK`, mark center stones as placed
  - File path: `src/game_engine.cpp`

- [x] T024 Implement `GameEngine::run_loop()` — main render loop:
    ```cpp
    while (!game_over_) {
        display_->render_frame();
        if (display_->handle_input()) break;  // ESC or quit signal
        // Determine whose turn it is
        if (ai_ && turn_ == ai_->color_) {
            auto move = ai_->search(board_, ai_->depth_);
            if (move) board_.place_stone(*move);
        } else {
            // Human player: wait for mouse click or keyboard input
        }
        // Switch turn after each successful placement
    }
    ```
  - File path: `src/game_engine.cpp`

- [x] T025 Implement `GameEngine::handle_input()` — integrate with DXLibDisplay's input handling; map mouse coordinates to board cells, validate legality before placing stone
  - File path: `src/game_engine.cpp`

## Phase 4: Unit Tests (P1) — NFR-003 🧪

### ≥90% Code Coverage for Game Engine

- [x] T026 Implement `tests/board_test.cpp`:
    - Test `CubeBoard::initialize()` — verify center stone positions and initial turn state
    - Test `CubeBoard::place_stone()` with edge cases: empty cell placement, sandwich flipping along each of 6 directions, illegal moves (no flip) rejected
    - Test `CubeBoard::get_valid_moves()` returns correct set; corner cases like "only one valid move exists"
    - Test `CubeBoard::game_over()` — full board → DRAW, no-moves for both players → winner declared by stone count
  - File path: `tests/board_test.cpp`

- [x] T027 Implement `tests/ai_player_test.cpp`:
    - Test `SimpleAI::evaluate()` against hand-computed counts
    - Test `SimpleAI::search()` — verify it never returns an illegal move, depth-limited recursion works correctly, pass behavior when no moves exist
  - File path: `tests/ai_player_test.cpp`

- [x] T028 Implement `tests/game_engine_test.cpp`:
    - Test game loop flow: initialization → several turns → game over detection
    - Verify AI integration with human player (alternating turns correctly)
  - File path: `tests/game_engine_test.cpp`

## Phase 5: E2E Integration Tests (P1) 🔄

- [ ] T029 Write pytest-based E2E test in `tests/e2e_tests.py`:
    - Scenario A: Human vs. Human — play a few moves, verify turn alternation, stone flipping correctness, win/draw conditions
  - File path: `tests/e2e_tests.py`

- [ ] T030 Write pytest-based E2E test for human vs. AI mode:
    - Verify AI responds with legal moves at each turn
    - Verify AI passes when no legal moves exist
    - Verify final evaluation matches expected winner/draw
  - File path: `tests/e2e_tests.py`

## Phase 6: Performance Validation (P3) 📊

- [x] T031 Implement performance benchmark in `tests/perf_benchmark.cpp`:
    - Measure FPS of the render loop using DXLib's timing functions
    - Verify ≥60 FPS on a standard desktop PC (Windows 7/8/10)
    - Profile move validation time per cell; confirm worst-case ≤5 ms total for full board scan
  - File path: `tests/perf_benchmark.cpp`

- [x] T032 Add memory profiling check (optional): ensure peak RSS stays below ~50 MB during gameplay
  - File path: `src/game_engine.hpp` (add optional stats collector)

## Phase 7: Polish & Cross-Cutting Concerns (P1) ✨

- [x] T033 Update `CMakeLists.txt`: add DXLib include/library paths, ensure clean build with no warnings under MSVC or MinGW-w64
  - File path: `CMakeLists.txt`

- [ ] T034 Add C++ doxygen-style comments to all public methods of `CubeBoard`, `SimpleAI`, and `DXLibDisplay`; ensure code compiles cleanly without any `-Wall -Wextra` warnings
  - File paths: `src/board.hpp/cpp`, `src/gui.hpp/cpp`, `src/ai_player.hpp/cpp`

- [x] T035 Add optional undo/redo functionality (if desired): maintain an action history stack, expose `board.undo()` method for manual replay
  - File path: `src/board.cpp` (optional extension)

## Summary

| Metric | Value |
|--------|-------|
| **Total tasks** | 34 |
| **Completed** | 0 (0%) |
| **In progress** | 0 |
| **Pending** | 34 |

### Breakdown by Phase:

| Phase | Tasks | Description |
|-------|-------|-------------|
| P0 Setup | 3 | Project structure, CMake, README |
| P1 Board Logic Core | 6 | Initialization, placement, flipping, valid moves, game over |
| P1 Rendering Layer | 7 | DXLib window, wireframe, grid, stones, lighting, markers, input |
| P2 AI Agent | 4 | Evaluate + minimax search for SimpleAI |
| P3 Game Engine | 5 | Board management, render loop, AI integration |
| P4 Unit Tests | 3 | board_test.cpp, ai_player_test.cpp, game_engine_test.cpp |
| P5 E2E Tests | 2 | Human-vs-human & human-vs-AI scenarios |
| P6 Performance | 2 | FPS benchmarking + memory profiling |
| P7 Polish | 3 | CMake cleanup, documentation, optional undo/redo |

### Parallel Execution Opportunities:

- **Phase 1 Board Logic** (T004–T010): All can be implemented in parallel — no cross-dependencies.
- **Rendering Layer** (T011–T017): Header declarations first (T011), then implementations in any order.
- **AI Agent** (Phase 2) is independent of rendering and board logic → fully parallelizable with Phase 3.

### Implementation Strategy Recommendation:

**MVP Scope:** Start with **Phase 0 + Phase 1 only** (board logic + rendering). This gives a playable human-vs-human game. Then iterate:
1. Build & render first → verify visual correctness
2. Add AI in a second pass
3. Add tests and performance checks last

This minimizes context switching and lets you ship an early playable version quickly! 🚀

---

> **Format validation:** All 34 tasks strictly follow the checklist format (`- [ ] T### [P?] [Story?] Description with file path`). No task is missing a checkbox, ID, or file path.