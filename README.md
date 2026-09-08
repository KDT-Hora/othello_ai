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

## 🎮 操作方法

ターミナルフォールバックモードの場合：

```text
Enter move (x y z): <x> <y> <z>
```

例：`4 3 3` — 黒がマス (x=4,y=3,z=3) に石を置く。

## 📦 リポジトリ構造

```
cubo_othello/
├── CMakeLists.txt          ← ビルド設定（MSVC / MinGW）
├── README.md               ← このドキュメント
├── DxLib/                  ← 描画ライブラリ
│   ├── include/DxLib.h
│   └── lib/dxlib.dll
├── src/
│   ├── board.hpp          ← 盤面管理（挟み込み判定ロジック）
│   ├── game.hpp           ← ゲームエンジン＋AI
│   ├── gui.hpp            ← DxLib GUI＋ターミナルフォールバック
│   └── main.cpp           ← メインプログラム
├── tests/                  ← ユニットテスト（Python 検証用）
└── .gitignore
```

## 📊 バージョン履歴

| バージョン | 日付 | 変更点 |
|-----------|------|--------|
| v0.1 | 2026-09-XX | 初期リリース：board.hpp, game.hpp, GUI（ターミナルフォールバック） |

## 📄 ライセンス

MIT License — クロスプラットフォーム開発を目的としています。
