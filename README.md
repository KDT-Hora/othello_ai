# Cube Othello (6×6×6) — C++20 / DXLib

3D 拡張オセロ。6×6×6 の立方体（216マス）上で、26方向（縦・横・斜め）の挟み込みでプレイする。
盤面ロジック + 深さ3のミニマックス AI + DXLib による回転可能な3D透視投影レンダリングで構成される。
詳細な仕様は [`specs/cubo-othello/spec.md`](specs/cubo-othello/spec.md) を参照。

## 📁 プロジェクト構成

```
CMakeLists.txt         # ビルド設定 (C++20 / vcpkg manifest / DXLib)
vcpkg.json              # vcpkg 依存関係 (gtest)
src/
  board.hpp/.cpp        # CubeBoard — 盤面状態とルール（純粋ロジック、DXLib非依存）
  ai_player.hpp/.cpp     # SimpleAI — 深さ限定ミニマックス（枝刈りなし、石数差評価）
  display.hpp/.cpp       # DXLibDisplay — 回転可能な3D透視投影レンダリング + 入力処理
  game_engine.hpp/.cpp   # GameEngine — 手番管理・パス判定・メインループ
  main.cpp               # WinMain エントリポイント
tests/
  board_test.cpp          # CubeBoard の単体テスト（DXLib不要）
  ai_player_test.cpp       # SimpleAI の単体テスト（DXLib不要）
third_party/DxLib/       # DXLib本体（.gitignore対象、下記手順で配置）
```

## 🛠 ビルド環境

- **Visual Studio 2022 以降 (Community可)** — C++ デスクトップ開発ワークロード（MSVC, CMake, vcpkg 同梱）
- DXLib は公式に MSVC 向けであり、MinGW 等との互換性は非公式・不安定なため MSVC を使用する。

### DXLib の配置

1. https://dxlib.xsrv.jp/ の「DXライブラリのダウンロード」から Visual Studio (C++) 用最新版 zip を取得。
2. 展開後の `DxLib_VC/プロジェクトに追加すべきファイル_VC用/` フォルダの中身一式を、このリポジトリの
   `third_party/DxLib/` にコピーする（`DxLib.h` が `third_party/DxLib/DxLib.h` に来るように）。
3. `third_party/DxLib/` は `.gitignore` 済みなのでコミットされない。

DXLib が見つからない場合、CMake は警告を出して `cubo_othello` (実ゲーム本体) のビルドをスキップし、
`cubo_tests`（盤面/AIロジックの単体テスト）だけをビルドする。

## 🏗 ビルド & テスト

Visual Studio に同梱の vcpkg をツールチェーンとして使う（`gtest` はマニフェストモードで自動取得）。

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE="C:\Program Files\Microsoft Visual Studio\18\Community\VC\vcpkg\scripts\buildsystems\vcpkg.cmake"

cmake --build build --config Debug

ctest --test-dir build -C Debug --output-on-failure
```

DXLib が `third_party/DxLib/` に正しく配置されていれば `cubo_othello.exe` も同時にビルドされる。

## ▶ 実行

```powershell
build\Debug\cubo_othello.exe             # モード選択メニューを表示（二人対戦 / AI対戦・黒 / AI対戦・白 / AI同士の観戦、AIの強さ3段階）
build\Debug\cubo_othello.exe --ai        # メニューを飛ばして 人(黒) vs AI(白, 深さ3) で開始（--ai=black で AI が黒）
```

操作:
- 左クリック: 合法手（緑の輪）に着手。カーソルが乗った手は黄色でハイライト
- 右ドラッグ / 矢印キー: 視点の回転、`V`: 視点リセット
- `U`: 一手戻す、`R`: 盤面リセット、`M`: モード選択メニューへ、`ESC`: 終了

## 📜 License

MIT
