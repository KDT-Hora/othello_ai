---
name: "Cube Othello: C++/DXLib の実装に関する仕様書について"
about: C++ 版 Cube Othello の仕様書（設計書）を作成し、GitHub に公開します。
labels: [enhancement, documentation, design]
assignees: koyo
---

## 📋 Issue の概要

Cube Othello（8×8×3 の立方体オセロ）を **C++ + DXLib** で実装するプロジェクトです。  
Python 版は存在しないため、ゼロから C++ で新規開発を行います。

---

## 🎯 目的

- **CMake** を使用したクロスプラットフォームな C++ プロジェクト構築
- **DXLib**（高レベルなゲームライブラリ）を使用した等角投影の描画
- **gtest** を使用した TDD（テスト駆動開発）による品質保証
- 簡易 AI：石数のみを評価するミニマックスアルゴリズム

---

## 📁 プロジェクト構成

```text
cubo_othello_cpp/
├── CMakeLists.txt                 # ビルド設定（DXLib + gtest）
├── README.md                      # 読み方・ビルド手順
│
├── src/                           # ゲームエンジン本体
│   ├── main.cpp                   # WinMain + メインループ
│   │
│   └── game_engine/
│       ├── game_engine.hpp        # GameEngine クラス（宣言）
│       ├── board.hpp              # CubeBoard クラス（宣言）
│       ├── board.cpp              # 盤面管理・挟み込み判定
│       ├── ai.hpp                 # SimpleAI クラス（宣言）
│       └── ai.cpp                 # ミニマックス AI
│
├── include/                       # DXLib ラッパー
│   └── dxlib_display.hpp          # 等角投影 GUI
│
└── tests/                        # ユニットテスト
    ├── CMakeLists.txt
    ├── main.cpp
    └── unittests/
        ├── board_tests.cpp        # CubeBoard の単体テスト
        └── ai_tests.cpp           # SimpleAI の単体テスト

e2e/                             # E2E 統合テスト（別途作成）
├── CMakeLists.txt
└── main.cpp
```

---

## 📐 クラス設計図

### CubeBoard

| メンバ変数 | 型・説明 |
|------------|----------|
| `grid_[z][y][x]` | int32: -1=空、0=B(黒)、1=W(白) [8×8×8] |
| `turn_` | int8: -1=未開始、0=BLACK、1=WHITE |
| `game_over_` | bool |
| `result_code_` | int: 0=in_progress, 1=BLACK_WINS, 2=DRAW, 3=NO_MOVES |

#### メソッド

```cpp
// Initialize the board with center pieces
void initialize();

// Place a stone and flip opponent's stones along all 6 directions
int place_stone(int x, int y, int z, int color);

// Return list of valid moves for the given player
std::vector<std::tuple<int,int,int>> get_valid_moves(int color);

// Count pieces of the given color
int count_pieces(int color) const;

// Check if game is over (full board or no moves)
bool is_game_over() const;
```

### SimpleAI

| メンバ変数 | 型・説明 |
|------------|----------|
| `color_` | int: BLACK/WHITE |
| `depth_` | int8: default=3 (search depth) |

#### メソッド

```cpp
// Evaluate board using simple heuristic: black_count - white_count
int evaluate(const CubeBoard& board);

// Minimax search with alpha-beta pruning, depth-limited to 3
std::tuple<int,int,int> search(const CubeBoard& board, int max_depth, int alpha, int beta);
```

### GameEngine

| メンバ変数 | 型・説明 |
|------------|----------|
| `board_` | std::unique_ptr<CubeBoard> |
| `turn_` | int8 |
| `game_over_` | bool |

#### メソッド

```cpp
// Start a new game (initializes board with center pieces)
void start();

// Main game loop: one turn at a time
void run_loop();

// Check game over condition and determine result
int check_game_over();

// Get the best move for the current player (with AI or random if no moves)
std::tuple<int,int,int> get_best_move(int color, int max_depth=3);
```

### DXLibDisplay （DXLib ラッパー）

| メソッド | 説明 |
|----------|------|
| `init()` | DXLib の初期化、ウィンドウ作成（120×80×480px） |
| `render_frame()` | フレームごとの描画処理（背景→盤面→石→有効手マーク） |
| `draw_cube_faces()` | 立方体の 8 つの面の枠線を引く |
| `draw_grid()` | グリッド線を描く |
| `draw_stones()` | z=0..7 の全層で石を描画（楕円＋陰影） |
| `mark_valid_moves()` | 有効手に「+」マークを表示 |
| `handle_input()` | キー入力とマウスクリックの処理 |

**等角投影の座標変換:**

```cpp
// 等角投影（斜め上からの視点）
int project_x(int x, int y) { return static_cast<int>(x * 0.5f - y * 0.268f); }
int project_y(int y, int z)    { return static_cast<int>(y * 0.5f + z * 0.268f); }

// 奥行きによる明度変化（前面が明るく、後面が暗く）
float get_ambient_light(int z) { return std::max(0.3f, 1.0f - static_cast<float>(z)/7.0f * 0.5f); }
```

---

## 🛠️ DXLib ラッパー設計

Cube Othello の等角投影は、DXLib の 2D グラフィック機能だけで完結します。

**include/dxlib_display.hpp:**

```cpp
#pragma once
#include <dxlib.h>
#include "game_engine/board.hpp"

class DXLibDisplay {
public:
    // --- Initialization ---
    void init(int width = 120, int height = 80);

    // --- Rendering ---
    void render_frame() const;
    void draw_cube_faces() const;      // 立方体の枠線
    void draw_grid() const;             // グリッド線
    void draw_stones(const CubeBoard& board) const;   // z=0..7 の石描画
    void mark_valid_moves(const CubeBoard& board, int color) const;

    // --- Input handling ---
    bool handle_input();                // キー入力（ESC: 終了、R: リセット）
    bool is_game_running() const;       // ゲーム進行中か？
};
```

---

## 📊 テスト戦略

### ユニットテスト（gtest）

| テスト対象 | カバレッジ目標 |
|------------|---------------|
| `CubeBoard` | ≥90% （置石・挟み込み・ターン交代・ゲームオーバー判定） |
| `SimpleAI`  | ≥85% （評価関数・ミニマックス検索） |
| `GameEngine`| ≥80% （ゲームループ全体） |

### E2E テスト（独自フレームワーク）

以下のシナリオを網羅：

- [ ] ゲーム初期化 → 中心部の 8 マスが配置される
- [ ] 黒のターンからの開始 → `turn = BLACK`
- [ ] 有効手の判定 → 「+」マークが表示されるマスが正しい
- [ ] 石の置換・挟み込み → `place_stone()` で挟まれた石がひっくり返る
- [ ] ターン交代 → 黒→白、白→黒で交替する
- [ ] パス判定 → 両方が置けない → ゲームオーバー
- [ ] ドロー判定 → 盤面埋まり（または石数差±16）

---

## 📝 ビルド手順

```bash
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release

ctest           # ユニットテスト実行
ctest --output-on-failure
```

### カバレッジ計測（gcov）

```bash
# Coverage enabled build: cmake .. -DCMAKE_CXX_FLAGS="-fprofile-arcs -ftest-coverage"
cmake .. -DCMAKE_CXX_FLAGS="-fprofile-arcs -ftest-coverage" -G "Visual Studio 17 2022"
cmake --build . --config Release

gcov src/game_engine/board.cpp tests/unittests/board_tests.cpp > board_coverage.txt
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_report
```

---

## 📅 作業ステップ

1. ✅ **CMakeLists.txt の作成**（DXLib + gtest 設定）
2. ⬜ `src/game_engine/board.cpp` と `board.hpp` の実装 → ユニットテスト
3. ⬜ `src/game_engine/ai.cpp` と `ai.hpp` の実装 → ユニットテスト
4. ⬜ `include/dxlib_display.hpp` の作成
5. ⬜ `src/main.cpp` で統合（DXLibDisplay + GameEngine）
6. ⬜ E2E テストの作成と実行
7. ⬜ ドキュメント補完

---

## 🔗 参照資料

- [Cube Othello の仕様書](../specifications/cube-othello-spec.md)
- [DXLib ドキュメント](https://www.fmarcelino.com/DXLib/)
- C++17 標準ライブラリ（`<vector>`, `<tuple>`, `<optional>`, `<memory>` など）

---

## 📋 リンク一覧

- [プロジェクトの README.md](../README.md) — プロジェクト概要・使い方
- [CMakeLists.txt](../CMakeLists.txt) — ビルド設定ファイル
- [doc/CPP_MIGRATION_PLAN.md](../doc/CPP_MIGRATION_PLAN.md) — C++ 移行計画詳細

---

## ✅ チェックリスト

- [x] Issue テキストの作成
- [ ] GitHub の Issues ページで Issue を作成（手動）
- [ ] リンクの検証
- [ ] 翻訳と校正
