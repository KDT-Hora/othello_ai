# Issue #1: Cube Othello — C++/DXLib 実装の設計文書 ✨

**ステータス:** ✅ 完了  
**作成日:** 2026-09-15  
**責任者:** koyo

---

## 📋 概要

Cube Othello（8×8×3 の立方体オセロ）を **C++ + DXLib** で実装するプロジェクトの設計文書です。

Python 版は存在しないため、ゼロから C++ で新規開発を行います。  
**TDD（テスト駆動開発）** を採用し、すべてのクラスに対して単体テストと E2E テストを網羅的に作成します。

---

## 🎯 品質目標

| 項目 | 目標値 |
|------|--------|
| ユニットテストカバレッジ | **≥90%**（`gcov` / `lcov` で測定） |
| E2E テスト | 全シナリオ網羅（置石、ターン交代、挟み込み、ゲームオーバーなど） |
| クラス設計 | オブジェクト指向（単一責任の原則） |
| ドキュメント | Doxygen スタイルコメント付与 |

---

## 🏗️ プロジェクト構造

```text
cubo_othello_cpp/
├── CMakeLists.txt                 # ビルド設定（DXLib、gtest 含め）
├── README.md                      # プロジェクト概要・ビルド手順
│
├── src/                          # ゲームエンジン本体
│   ├── main.cpp                   # WinMain + メインループ
│   │
│   └── game_engine/
│       ├── game_engine.hpp        # GameEngine クラス（宣言）
│       ├── board.hpp              # CubeBoard クラス（宣言）
│       ├── board.cpp              # 盤面管理・挟み込み判定
│       ├── ai.hpp                 # SimpleAI クラス（宣言）
│       └── ai.cpp                 # ミニマックス AI
│
├── include/                      # DXLib ラッパー
│   └── dxlib_display.hpp          # 等角投影 GUI
│
└── tests/                       # ユニットテスト
    ├── CMakeLists.txt            # テストビルド設定
    ├── main.cpp                  # テストエントリーポイント
    └── unittests/
        ├── board_tests.cpp       # CubeBoard クラスの単体テスト
        ├── ai_tests.cpp          # SimpleAI クラスの単体テスト
        └── game_engine_tests.cpp # GameEngine の単体テスト

e2e/                            # E2E 統合テスト（別途作成）
├── CMakeLists.txt               # E2E テストビルド設定
└── main.cpp                    # E2E テストエントリーポイント
```

---

## 🧪 TDD ワークフロー

各クラスの実装手順：

1. **赤いテストを書く** → `tests/unittests/<module>_tests.cpp` にテストコードを記述
2. **コンパイルエラーが出るか、既存のビルドで失敗することを確認**
3. **「動く」ように最小限の実装する** → `src/game_engine/<module>.cpp` を書く
4. **緑にさせる**（`cmake --build . -t test` で全テストパス）
5. **リファクタリング**（設計の改善）

> ✅ 1 つのクラスの実装が終わるごとに、そのクラスのテストカバレッジが「90%」を超えることを確認する。

---

## 📊 テストのカバレッジ目標

| モジュール | メソッド数 | 単体テスト数（最低） |
|-----------|-----------|---------------------|
| `CubeBoard` | ~15 | ≥12 |
| `SimpleAI` | ~6 | ≥5 |
| `GameEngine` | ~8 | ≥7 |

> 合計で **≥90%** のカバレッジを達成（`gcov` で測定）

---

## 🔬 E2E テストのシナリオ（網羅）

以下のシナリオすべてでテストがパスすることを目指す：

| # | シナリオ名 | 説明 |
|---|-----------|------|
| 1 | **初期化** | `initialize()` で中心部 8 マスが配置されること |
| 2 | **黒のターンからのゲーム開始** | `turn = BLACK` になること |
| 3 | **有効手の判定（+マーク表示）** | 挟み込み可能なマスに「+」が表示されること |
| 4 | **石の置換と挟み込み** | `place_stone()` で正しい挟み込みが行われること |
| 5 | **ターン交代** | 挟み込み後、ターンが白へ切り替わること |
| 6 | **無手の判定（パス）** | 置ける手がなくなること → ゲームオーバー |
| 7 | **ドロー（盤面埋まり）** | `is_full()` でゲーム終了すること |
| 8 | **AI の最善手選択** | depth=3 のミニマックスで最適な手を返すこと |
| 9 | **結果コードの出力** | BLACK WINS / DRAW / NO MOVES が正しく判定されること |

---

## 📐 クラス設計図（詳細）

### CubeBoard クラス

- `grid_[z][y][x]`：3 レーヤー×8×8 の 3D グリッド（`int` で `-1/0/1`）
- `turn_`：現在のターン（`-1`=未開始、`0`=BLACK、`1`=WHITE）
- `game_over_`：ゲーム終了フラグ
- `result_code_`：結果コード（`0`=継続中、`1`=BLACK 勝、`2`=ドロー、`3`=NO MOVES）

**メソッド：**

| メソッド | 説明 |
|----------|------|
| `initialize()` | 中心部 (x,y,z)∈{3,4}×{3,4}×{3,5} に黒・白を配置 |
| `place_stone(x,y,z,color)` | 石を置き、6 方向の挟み込みを実行して返す |
| `get_valid_moves(color)` | その色の有効手のリストを返す |
| `count_pieces(color)` | その色の石数をカウントする |

### SimpleAI クラス

- `color_`：担当するプレイヤー（BLACK/WHITE）
- `depth_`：探索深さ（デフォルト 3）

**メソッド：**

| メソッド | 説明 |
|----------|------|
| `evaluate(board)` | 自分の石 − 相手の石を計算して返す |
| `search(board, max_depth=3)` | depth=3 のミニマックスで最善手を探索し返す |

### GameEngine クラス

- ゲームループの制御を担当。`turn_`、`game_over_` を管理する。

**メソッド：**

| メソッド | 説明 |
|----------|------|
| `start()` | 盤面初期化（中心部配置） |
| `run_loop()` | ゲームループ（ターン交代 → 石置換 → ターン切り替え → ゲームオーバー判定） |
| `check_game_over()` | `is_full()` または「無手」で終了判定 |

### DXLibDisplay クラス

- DXLib の高レベルラッパー。等角投影・明度変化の描画を担当する。

**メソッド：**

| メソッド | 説明 |
|----------|------|
| `init()` | DXLib の初期化、ウィンドウ作成（120×80×480px） |
| `render_frame()` | フレームごとの描画処理（背景→盤面→石→有効手マーク） |
| `draw_cube_faces()` | 立方体の 8 つの面の枠線を引く |
| `draw_grid()` | グリッド線を描く |
| `draw_stones(board)` | z=0..7 の全層で石を描画（楕円＋陰影） |
| `mark_valid_moves(board, color)` | 有効手に「+」マークを表示 |
| `handle_input()` | キー入力とマウスクリックの処理 |

---

## 📝 ビルド手順

```bash
# Windows: Visual Studio（推奨）
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release

ctest               # ユニットテスト実行
ctest --output-on-failure

# カバレッジ計測
gcov -o build/Release --object-directory=../build/Release \
    src/game_engine/board.cpp tests/unittests/board_tests.cpp ...
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_report
```

> ✅ 目標：`coverage_report/index.html` で **≥90%** のカバレッジを達成する。

---

## 🔗 リンク一覧

- [Cube Othello の仕様書](../specifications/cube-othello-spec.md) — ゲームルール・仕様
- [feature-001-spec.md](../specifications/feature-001-spec.md) — 機能仕様詳細
- [DXLib ドキュメント](https://www.fmarcelino.com/DXLib/)
