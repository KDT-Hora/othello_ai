# Cube Othello (C++ / DxLib)

8×8×8 の立方体盤面を持つオセロゲーム。DxLib を用いた 3D グリッド描画と、ターミナルフォールバック（絵文字表示）の両モードをサポートします。

## 📋 仕様概要

| 項目 | 内容 |
|------|------|
| **盤面形状** | 8×8×8 の立方体（512 マス） |
| **初期配置** | 中心部 (3,3)〜(4,4) に黒・白を隣り合うように配置 |
| **挟み込み判定** | 6 方向（±x, ±y, ±z）、斜めは考慮しない |
| **ターン交代** | 黒が先攻。有効な手がない場合はパス判定 |
| **ゲーム終了** | 盤面埋まり、または両プレイヤーとも置けない状態 |
| **AI** | シンプルミニマックス（石数差のみ評価） |

## 🛠️ ビルド手順

### Windows (MSVC++ / Visual Studio 2022)

```powershell
cd ~/.hermes/skills/project-cubo_othello
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022"
msbuild CubeOthello.sln /p:Configuration=Release
cubo_othello.exe
```

### Windows (MinGW-w64)

```powershell
# MinGW-w64 をインストール（choco または MSYS2）
choco install mingw

cd ~/.hermes/skills/project-cubo_othello
mkdir build && cd build
cmake .. -G "MinGW Makefiles"
mingw32-make
cubo_othello.exe
```

### ターミナルフォールバックモード（DxLib 不要）

```powershell
# 絵文字表示で動作する簡易版
cubo_othello.exe --terminal -v
```

## 🧪 テスト実行とカバレッジ確認

### C++ ユニットテスト（Google Test）

```bash
cmake .. -DBUILD_TESTS=ON -DENABLE_GTEST=ON
msbuild CubeOthello.sln /p:Configuration=Debug,Platform=x64
ctest -C Debug --output-on-failure
```

カバレッジレポートの生成：

```bash
pip install pytest pytest-cov coverage
pytest --cov=cubo_othello --cov-report=html:build/coverage/htmlcov tests/e2e_tests.py
# 結果： build/coverage/htmlcov/index.html で確認可能
```

### カバレッジ目標

- **C++ ユニットテスト**: 90%+（Google Test の分岐カバレッジ）
- **Python E2E テスト**: 100%（ボードロジックの全パス網羅）

## 📐アーキテクチャ

```
cubo_othello/
├── CMakeLists.txt          ← ビルド設定（MSVC / MinGW、Google Test 統合）
├── README.md               ← このドキュメント
├── DxLib/                  ← 描画ライブラリ
│   ├── include/DxLib.h
│   └── lib/dxlib.dll
├── src/
│   ├── board.hpp          ← 盤面管理（挟み込み判定ロジック）
│   ├── game.hpp           ← ゲームエンジン＋AI（簡易ミニマックス）
│   ├── gui.hpp            ← DxLib GUI＋ターミナルフォールバック
│   └── main.cpp           ← メインプログラム（引数処理、ゲームループ）
├── tests/
│   ├── unit_tests.cpp     ← C++ Google Test ユニットテスト（カバレッジ 90%+）
│   ├── e2e_tests.py       ← Python E2E テスト（pytest + coverage）
│   └── coverage_report.py ← カバレッジレポート生成スクリプト
└── .gitignore
```

## 🧪 テストマトリクス

| Test Suite | フレームワーク | コVERAGE 目標 |
|---|---|---|
| Board::flip_stones | C++ Google Test | ✓ 全方向・全パターン網羅 |
| GameEngine::play_move | C++ Google Test | ✓ ターン交代・パス判定・終了条件 |
| SimpleAI::search | C++ Google Test | ✓ depth=0~3 の探索パス網羅 |
| CubeGridRenderer | C++ Google Test | ✓ DxLib モード＋ターミナルフォールバック両方 |
| FullGameFlow | Python pytest | ✓ ゲームループ全体の統合テスト |

## 📊 バージョン履歴

| バージョン | 日付 | 変更点 |
|-----------|------|--------|
| v0.1 | 2026-09-XX | 初期リリース：board.hpp, game.hpp, GUI（ターミナルフォールバック） |
| v0.2 | 2026-09-XX | **TDD 追加**: ユニットテスト（Google Test）、E2E テスト（pytest）、カバレッジレポート生成 |

## 📄 ライセンス

MIT License — クロスプラットフォーム開発を目的としています。
