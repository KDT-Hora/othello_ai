// Cube Othello — Game Engine Module (C++17 / Terminal Fallback Mode)
#pragma once

#include <memory>
#include "board.hpp"   // ← 相対パスは親ディレクトリから検索可能に

namespace cubo {

/**
 * @brief SimpleAI クラスの forward declaration（game_engine.hpp 側で定義済み）
 */
class SimpleAI;


// =============================================================================
// GameEngine — ゲーム制御クラス（ターミナルフォールバックモード）
// =============================================================================

class GameEngine {
public:   // ★全てのメンバを public にして main.cpp でアクセス可能に
    CubeBoard board_;          // 盤面データ
    PieceColor turn_player_ = BLACK;   // 現在のターン（黒が先攻）
    bool game_over_ = false;
    std::string winner_msg_;
    std::unique_ptr<SimpleAI> ai_;     // AI プレイヤー

private:
    int game_result_;           // 結果コード：0=継続、1=終了、2=ドロー、3=ゲームオーバー（無手合）

public:
    GameEngine();
    ~GameEngine() = default;

    /** @brief AI プレイヤーを設定する。 */
    void set_ai(std::unique_ptr<SimpleAI> ai);

    /** @brief ゲーム開始処理（初期配置）。 */
    void start();

    /** @brief ゲームオーバー判定。 */
    bool check_game_over() const;

    /** @brief ゲーム結果を表示する（勝利メッセージなど）— non-const に変更 */
    void render_final_score();   // ★ const を削除：文字列リテラル代入のため

    /** @brief ゲームループを実行し、結果を返す。 */
    int run();

public:
};


// =============================================================================
// GameEngine 実装 — 全て inline で定義（cpp ファイル不要）
// =============================================================================

inline GameEngine::GameEngine() {
    board_ = CubeBoard();
    turn_player_ = BLACK;   // 黒が先攻
    game_over_ = false;
    winner_msg_.clear();
    game_result_ = 0;       // 未実行
}

inline void GameEngine::set_ai(std::unique_ptr<SimpleAI> ai) {
    ai_.reset(ai.release());   // move semantics: pointer を渡すだけで OK
}

inline void GameEngine::start() {
    board_ = CubeBoard();      // 初期配置
    turn_player_ = BLACK;
    game_over_ = false;
    winner_msg_.clear();
    game_result_ = 0;
}

inline bool GameEngine::check_game_over() const {
    auto moves = board_.get_all_valid_moves();
    return moves.empty();      // 有効な手が存在しない → ゲームオーバー
}

// ★ render_final_score を non-const に：文字列リテラル代入のため
inline void GameEngine::render_final_score() {
    auto [black_count, white_count] = board_.count_pieces();
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  BLACK:   " << black_count << " stones\n";
    std::cout << "  WHITE:   " << white_count << " stones\n";

    if (black_count > white_count) {
        winner_msg_ = "BLACK WINS!";
    } else if (white_count > black_count) {
        winner_msg_ = "WHITE WINS!";
    } else {
        winner_msg_ = "DRAW (同点)";
    }

    std::cout << "  Result:   " << winner_msg_ << "\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━\n\n";
}

inline int GameEngine::run() {
    constexpr int MAX_PLY = 1024;   // マックス手数（ループ防止）
    int ply_count = 0;

    while (!game_over_ && ply_count < MAX_PLY) {
        if (board_.is_full()) {
            game_result_ = 3;       // ドロー
            break;
        }

        // ターン表示
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━\n";
        std::cout << "  Turn:     ";
        if (turn_player_ == BLACK) std::cout << "(BLACK)\n";
        else if (turn_player_ == WHITE) std::cout << "(WHITE)\n";
        std::cout << "  Valid moves: " << board_.get_all_valid_moves().size() << "\n";

        // ゲームオーバーチェック
        auto valid_moves = board_.get_all_valid_moves();
        if (valid_moves.empty()) {
            game_result_ = 2;       // ゲームオーバー（どちらの石も置けない）
            break;
        }

        // 入力処理（簡易：ランダム選択 — Q10: AI は後で追加）
        int x, y;
        if (valid_moves.size() == 1) {
            x = static_cast<int>(valid_moves[0].first);
            y = static_cast<int>(valid_moves[0].second);
        } else {
            // ランダム選択（簡易）
            int idx = 0;   // TODO: ユーザー入力から変更可能に
            x = static_cast<int>(valid_moves[idx].first);
            y = static_cast<int>(valid_moves[idx].second);
        }

        // 石を置く
        board_.set_cell(x, y, turn_player_);

        ply_count++;
        std::cout << "    Placed at (" << x << ", " << y << ") — ";
        if (turn_player_ == BLACK) std::cout << "Black\n";
        else std::cout << "White\n";

        // ターン交代
        turn_player_ = (turn_player_ == BLACK) ? WHITE : BLACK;
    }

    game_over_ = true;
    render_final_score();

    return game_result_;   // 0=継続中、1=終了、2=ドロー、3=ゲームオーバー（無手合）
}


} // namespace cubo
