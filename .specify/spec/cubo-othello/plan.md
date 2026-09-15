---
name: "Cube Othello — C++ DXLib Implementation Plan"
short-name: cubo-othello-plan
created-at: 2026-09-15
author: Hermes Agent (via /speckit-plan)
spec-ref: .specify/spec/cubo-othello/spec.md
---

# Cube Othello — C++ DXLib Implementation Plan 🏗️

## Executive Summary

This plan outlines the implementation of **Cube Othello**, a 3D Othello game played on an 8×8×8 cube, implemented in C++ with DXLib for rendering and gtest for unit testing. The project follows TDD-driven development (RED→GREEN→REFACTOR) and targets ≥90% test coverage.

**Technology Stack:**
- **Language:** C++17
- **Rendering:** DXLib 2.x (Windows-only, no OpenGL/SDL/SFML)
- **Testing:** GoogleTest + gcov/lcov for coverage reporting
- **Build System:** CMake 3.15+ with MSVC or MinGW-w64 g++

---

## Architectural Decisions

### Stack & Dependencies

| Component | Technology | Rationale |
|-----------|-----------|-----------|
| Game Logic | C++ STL (std::array, std::vector) | Zero dependencies for core logic; pure combinatorial game theory |
| Rendering | DXLib 2.x API | Lightweight, Windows-native, matches specification constraints |
| Testing | gtest + gcov/lcov | Industry-standard unit test framework with coverage metrics |

**Not used:** OpenGL, DirectX, SFML, SDL (explicitly excluded by spec).

### File Structure & Responsibilities

```text
.
├── CMakeLists.txt                 ← build configuration (DXLib paths)
├── README.md                      ← user-facing usage guide
├── src/
│   ├── main.cpp                   ← WinMain + game loop entry point
│   ├── board.hpp                  ← CubeBoard class declaration
│   └── board.cpp                  ← CubeBoard implementation: sandwich detection, flipping, valid moves
│   ├── ai_player.hpp              ← SimpleAI class declaration
│   └── ai_player.cpp              ← SimpleAI implementation: stone-count evaluation + minimax(d=3)
├── .specify/                      ← Speckit workflow artifacts
│   ├── spec/cubo-othello/spec.md  ← functional & non-functional requirements
│   └── spec/cubo-othello/plan.md  ← this document
├── tests/
│   ├── board_test.cpp             ← unit tests for CubeBoard (≥90% coverage)
│   └── ai_test.cpp                ← unit tests for SimpleAI
├── include/                       ← optional DXLib wrapper utilities
└── assets/                        ← resources (icon, sound effects)
```

### Class & Interface Design

#### `CubeBoard` — Game State Manager

**Responsibility:** Encapsulates the 8×8×8 cube board state and all game rules logic.

| Method | Purpose |
|--------|---------|
| `initialize()` | Place center stones at `(x,y,z) ∈ {3,4}×{3,4}×{3,5}`; set `turn = BLACK` |
| `place_stone(x, y, z, color)` | Validate move → flip sandwiched opponent stones along 6 axial directions → update state |
| `get_valid_moves(color) const` | Scan all empty cells; return those that produce ≥1 flip (legal moves only) |
| `game_over() const` | Determine end condition: full board (draw), no-moves for both players, or in-progress |
| `count_pieces() const` → `{black_count, white_count}` | Used by AI evaluation and win determination |

**Key invariant:** A move is illegal if it produces zero flips; such a cell must never be returned as valid.

#### `SimpleAI` — Minimax Player Agent

**Responsibility:** Select moves for the non-human player using depth-limited minimax search.

| Method | Purpose |
|--------|---------|
| `evaluate(const CubeBoard&) const` → int | Returns `my_stones − opponent_stones`; positive = favorable to self |
| `search(const CubeBoard&, ply) std::optional<Move>` | Minimax with depth limit; returns best move or `nullopt` if no legal moves exist |

**Design note:** The evaluation function is intentionally simple (stone count only). No positional heuristics are required per the spec. Pruning (alpha-beta) is omitted to keep code size and complexity minimal, consistent with "simple minimax AI" in the acceptance criteria.

#### `DXLibDisplay` — Rendering Layer

**Responsibility:** Wrap DXLib's 2D API to render the cube board in a flat-plane isometric-like projection.

| Method | Purpose |
|--------|---------|
| `init(width, height)` | Create DXLib window; set background color (`COLOR_BLACK = RGB(0x1C1C2A)`) |
| `render_frame()` | Clear → draw wireframe faces → draw grid lines → draw stones with depth lighting → render turn indicator / score |
| `draw_stones(const CubeBoard&)` | Render each cell as an ellipse; apply per-layer ambient occlusion (`light = 1.0 − z/7.0*0.5`) |

**Coordinate mapping (flat-plane isometric approximation):**
```text
cx_pos = cx + Px + x * GS   // rightward along the screen X axis
cy_pos = cy - g + (7-y) * GS // upward along the screen Y axis (y increases toward top-left)
light  = max(0.3f, 1.0f - static_cast<double>(z)/7.0*0.5f)
```

**Color palette:**
- Black stone: `RGB(0x2C2C4A)` — dark slate-gray ellipse
- White stone: `RGB(0xF5F0E6)` — warm off-white with subtle inner highlight

#### `GameEngine` — Game Loop Controller

**Responsibility:** Coordinate the game loop, input handling, and AI integration.

| Method | Purpose |
|--------|---------|
| `start()` | Initialize board; start turn at BLACK; mark center stones placed |
| `run_loop()` | Main render loop: handle input → validate move → place stone → switch turn → render |
| `handle_input()` | Process `ESC` (quit), mouse click (place on valid cell), `'R'` (reset board) |

**Input binding:**
- `ESC` → terminate game, call DXLib cleanup, exit
- Mouse click at `(screen_x, screen_y)` → map to nearest valid-move cell; if legal for current player, place stone immediately
- `'R'` key → reset board to initial state (does not end the game)

---

## Phases & Touch-points

### Phase 0: Board Logic Core (Priority: CRITICAL)

**Goal:** Implement the `CubeBoard` class with correct sandwich detection and flipping.

**Files created/modified:**
- `src/board.hpp` — declare `CubeBoard`, its state members, and public API
- `src/board.cpp` — implement all game rules methods

**Acceptance criteria from spec.md (FR-001 through FR-006):**
- `[ ]` Board initializes with correct center stones at `(3,3,3), ..., (4,4,4)` across layers 3 and 5.
- `[ ]` `place_stone` performs sandwich detection along all 6 directions; flips opponent stones only where a valid sandwich exists.
- `[ ]` `get_valid_moves` returns exactly the set of empty cells that produce at least one flip.
- `[ ]` Turn switches after each legal move; no-move condition triggers game over.

### Phase 1: Rendering Layer (Priority: HIGH)

**Goal:** Render the cube board using DXLib in a flat-plane isometric view with depth-based lighting.

**Files created/modified:**
- `src/gui.hpp` — declare `DXLibDisplay`, its dimensions, color constants
- `src/gui.cpp` — implement window creation, frame rendering, stone drawing with per-layer shading, valid-move markers ("+" symbol)

**Acceptance criteria from spec.md (FR-007 through FR-009):**
- `[ ]` Window opens at 120×80 pixels (or configurable); background is dark gray/black.
- `[ ]` The three visible cube faces are wireframed to convey pseudo-3D structure without true occlusion.
- `[ ]` Each cell displays a small ellipse labeled `"."`, `"B"`, or `"W"`; black stones use `#3a4a5e`, white stones use `#f0f0f5`.
- `[ ]` Valid-move cells show a bright green "+" marker above the stone/empty space.

### Phase 2: AI Agent (Priority: MEDIUM)

**Goal:** Implement a minimax-based AI that chooses moves based solely on stone count difference.

**Files created/modified:**
- `src/ai_player.hpp` — declare `SimpleAI` with member variables `color_`, `depth_` (default 3)
- `src/ai_player.cpp` — implement `evaluate()` and minimax search; handle pass condition when no legal moves exist

**Acceptance criteria from spec.md:**
- `[ ]` `evaluate` returns `my_stones − opponent_stones`.
- `[ ]` Minimax searches to depth 3 (plies) with no pruning.
- `[ ]` When multiple moves tie on evaluation, any one may be returned randomly.

### Phase 3: Game Engine & Integration (Priority: HIGH)

**Goal:** Wire all components together into a cohesive game loop that responds to user input and AI turns.

**Files created/modified:**
- `src/game_engine.hpp/cpp` — declare `GameEngine`, integrate with DXLibDisplay, handle turn switching, win detection, and final score display

### Phase 4: Unit Tests (Priority: CRITICAL)

**Goal:** Achieve ≥90% code coverage on game logic.

**Files created/modified:**
- `tests/board_test.cpp` — test suite for `CubeBoard`: initialization, valid move detection, stone flipping edge cases, game-over conditions
- `tests/ai_test.cpp` — test suite for `SimpleAI`: no-move pass behavior, tie-breaking randomness, depth-limited search correctness

### Phase 5: E2E Scenarios (Priority: MEDIUM)

**Goal:** Validate full gameplay scenarios end-to-end.

**Files created/modified:**
- `tests/e2e_tests.py` — pytest-based integration tests simulating human-vs-human and human-vs-AI sessions; assert turn alternation, stone flipping correctness, win/draw conditions

---

## Open Questions & Decisions Pending

| # | Question | Decision / Notes |
|---|-----------|------------------|
| 1 | DXLib source location on build machine? | Assume user has a local copy at `dxlib_libs/`; if not present, suggest downloading from GitHub or using the bundled version. |
| 2 | Should valid-move markers show for both players simultaneously, or only the current player's legal moves? | Spec says "for both players" — show "+" on all currently legal cells regardless of turn. |
| 3 | Reset key binding: should `'R'` reset to initial state or to previous undo state? | Spec implies full reset → `board.initialize()` without ending game. |

---

## Dependencies & Build Instructions

### Prerequisites

- CMake ≥ 3.15
- MSVC (Visual Studio 2022) **or** MinGW-w64 g++ with C++17 support
- DXLib library (Windows-only): either download from GitHub or point `DXLIB_PATH` to a local copy.

### Build Commands

```bash
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
```

Or with MinGW:
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make VERBOSE=1
```

### Test Build

```bash
cmake .. -DBUILD_TESTS=ON
cmake --build . --target tests
ctest
lcov --capture --output-file coverage.info
genhtml coverage.info --output-directory coverage_report
open coverage_report/index.html
```

---

## Glossary (for the plan document)

- **Cube Othello:** 3D extension of Reversi played on an 8×8×8 cube with stones placed at `(x,y,z)` coordinates.
- **Sandwich detection:** The rule that a move is legal only if it sandwiches opponent stones between the newly placed stone and another of the same player's color along one or more axial directions.
- **Flat-plane isometric projection:** A 2D rendering technique where the cube's three visible faces are projected onto a single plane using affine transforms; depth is conveyed via per-layer lighting rather than true occlusion.

---

> **Handoff note:** This plan document serves as the "source of truth" for implementation scope and technical decisions. Subsequent convergence (`/speckit-converge`) runs will compare the actual codebase against these artifacts to generate precise remaining work items.
