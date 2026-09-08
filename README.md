# Cube Othello (C++ / DxLib)

8×8×8 の立方体盤面を持つオセロゲーム。DxLib を用いて DirectX 9 ベースの 3D グリッド描画を行います。

## 📋 仕様概要

- **盤面形状**: 8×8×8 の立方体（512 マス）
- **初期配置**: (3,3)〜(4,4) の中心部で黒・白が隣り合うように配置
- **挟み込み判定**: x±, y±, z±の 6 方向のみ（斜めは考慮しない）
- **ゲーム終了条件**: 盤面埋まり または 置ける手のないプレイヤーがいる場合
- **勝敗判定**: 石数の多い方が勝ち
- **AI**: シンプルなミニマックス（評価関数＝自分の石−相手の石）

## 🛠️ ビルド環境要件

- Windows OS
- MSVC++ 19.x (Visual Studio 2022) または MinGW-w64 g++ 9.3+
- DirectX SDK (runtime のみ、開発用ライブラリは DxLib が提供）
- CMake 3.15+

## 📦 ビルド手順

### MSVC++（Visual Studio 2022）の場合

```powershell
# CMake を介して Visual Studio プロジェクトを生成
cmake -B build -S . -G "Visual Studio 17 2022" -DCMAKE_TOOLCHAIN_FILE=build/vcpkg.cmake -A Win64

# ビルド
cmake --build build --config Release
```

または、VS の「CMake」ツールからソリューションを生成してビルドします。

### MinGW-w64 の場合

```bash
mingw-w64-x86_64-g++ -std=c++17 \
  -Iinclude/ -IDxLib/include \
  -c src/main.cpp -o main.o && \
g++ main.o DxLib/lib/dxlib.a -o cubo_othello.exe
```

### CMake の場合（デフォルト）

```bash
cmake -B build -S .
cmake --build build
./build/cubo_othello.exe
```

## 🎮 操作方法

| キー | 動作 |
|------|------|
| `1`〜`9` | 盤面のマスを指定（座標入力） |
| `Enter` | 石を置く（有効な手なら置ける） |
| `Space` / `Enter` | AI のターンにスキップ |
| `R` | ゲームリセット |
| `Esc` | アプリを終了 |

盤面上では「+」印がついているマスが有効な手です。

## 📂 プロジェクト構造

```text
cubo_othello/
├── CMakeLists.txt          # ビルド設定（CMake）
├── DxLib/                   # DxLib ライブラリ一式
│   ├── include/DxLib.h     # ヘッダー
│   └── lib/dxlib.lib       # ランタイムリンク用ライブラリ
├── src/
│   ├── main.cpp            # エントリーポイント、ゲームループ
│   ├── board.hpp/.cpp      # 8×8×8 グリッド管理
│   ├── game.hpp/.cpp       # ゲーム状態機械（ターン管理）
│   ├── ai.hpp/.cpp         # シンプルミニマックス AI
│   └── gui.hpp/.cpp        # DxLib 描画ループ、等角投影
├── specifications/
│   └── cube-othello-spec.md  # ゲーム仕様書（最終版）
└── README.md
```

## 🧪 テスト

```bash
ctest -V
# または手動テスト:
# - 黒が 1 つ置ける手で盤面の中心を挟み込む → 2 個ひっくり返ることを確認
# - 双方とも置ける手がなくなるまでゲームを進め、勝敗判定が正しく動作するか確認
```

## 📄 ライセンス

Cube Othello は MIT ライセンスの下で公開されています。

DxLib（描画ライブラリ）については [dxlib.github.io](https://github.com/dxlib/DxLib) を参照してください。
