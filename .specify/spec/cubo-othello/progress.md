# Cube Othello — Progress (updated 2026-09-29)

C++20 / DXLib / MSVC (VS 2022) で実装済み。`cubo_othello.exe`, `cubo_tests.exe` (33 tests, all pass), `cubo_perf_benchmark.exe` がビルド可能。

## Done
- 盤面ロジック (`board.*`): 初期配置, 6方向の挟み, 合法手, 終局判定, 石数評価
- AI (`ai_player.*`): SimpleAI 深さ3ミニマックス (枝刈りなし, 石数差評価)
- 描画・入力 (`display.*`): 回転可能な3D透視投影 (右ドラッグ/矢印キー), 広い石間隔, 奥面のマス目グリッド, 合法手リング + ホバー強調, 1クリック1着手
- ゲーム進行 (`game_engine.*`, `game_loop.cpp`): 手番/パス/リセット/undo
- テスト: board 21 / AI 6 / engine 6, OpenCppCoverage ターゲット, 性能ベンチマーク

## Remaining
- 評価関数の強化 (連結石など)
- `AlphaBetaAI` (αβ枝刈り) と深さ4-5の探索
- AI 同士の総当たり対戦シナリオ
- 公開メソッドへの Doxygen コメント
