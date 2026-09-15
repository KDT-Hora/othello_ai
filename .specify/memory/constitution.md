---
name: Constitution for Cube Othello Implementation
version: 0.1.0
author: koyo
---

# 📜 プロジェクト憲章

## Mission

Cube Othello（8×8×3 の立方体オセロ）を C++ + DXLib で実装し、
TDD（テスト駆動開発）で品質を保証する。

## Core Principles (MUST)

- **Simplicity First**: 複雑なアルゴリズムは必要最小限に抑える。評価関数は「石数の差」のみで十分とする。
- **Test-Driven Development**: すべてのクラスに対して gtest で単体テストを作成し、カバレッジ≥90% を達成する。
- **Type Safety**: C++17 の std::optional, std::unique_ptr などを活用し、null pointer エラーを防止する。
- **DRY Principle**: 挟み込み判定やターン交代ロジックは共通関数として抽出する。

## Guidelines (SHOULD)

- クラス設計では単一責任の原則（SRP）に従う。
- DXLib の描画はすべて `dxlib_display.hpp` に集約する。
- テストはユニットテストと E2E テストを分けて管理する。

## Anti-Patterns (MUST NOT)

- 石の挟み込み判定で斜め方向を含めるな（6 軸のみ）
- ゲームループ内で UI とロジックが混在するな（GameEngine は UI から独立する）
- AI が深さ制限なしに探索するな（depth=3 で十分）
