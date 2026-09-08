// Cube Othello — Game Engine (C++ / DxLib)
// ============================================================================
// ゲーム状態管理、ターン交代、ゲーム終了判定、勝敗判定を扱う。

#pragma once

#include <array>
#include <vector>
#include <string>
#include "board.hpp"

namespace cubo {

constexpr int BOARD_SIZE = 8;
enum class PieceColor : uint8_t { EMPTY, BLACK, WHITE };

// ============================================================================
class CubeBoard {
public:
    using BoardGrid = std::array<std::array<PieceColor, BOARD_SIZE>, BOARD_SIZE>;
private:
    BoardGrid board_;

    void initialize() {
        for (int x = 0; x < BOARD_SIZE; ++x)
            for (int y = 0; y < BOARD_SIZE; ++y)
                board_[x][y].fill(PieceColor::EMPTY);

        constexpr int black_x = 3, white_x = 4;
        for (int y = 3; y <= 4; ++y) {
            for (int z = 3; z <= 4; ++z) {
                board_[black_x][y][z] = PieceColor::BLACK;
                board_[white_x][y][z] = PieceColor::WHITE;
            }
        }
    }

public:
    CubeBoard() : board_{} { initialize(); }
    ~CubeBoard() = default;

    char get_cell(int x, int y, int z) const {
        if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE ||
            z < 0 || z >= BOARD_SIZE) return '.';
        switch (board_[x][y][z]) {
            case PieceColor::EMPTY: return '.';
            case PieceColor::BLACK: return 'B';
            case PieceColor::WHITE: return 'W';
            default: return '?';
        }
    }

    bool set_cell(int x, int y, int z, PieceColor color) {
        if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE || z < 0 || z >= BOARD_SIZE)
            return false;
        if (board_[x][y][z] != PieceColor::EMPTY) return false;
        board_[x][y][z] = color;
        return true;
    }

    struct FlipResult {
        std::vector<std::array<int,3>> flipped;
        bool success = false;
    };

    FlipResult flip_stones(int x, int y, int z, PieceColor player_color) {
        FlipResult result{};

        constexpr struct Dir3D { int dx, dy, dz; } dirs[] = {
            { 1,0,0}, {-1,0,0}, { 0,1,0},{ 0,-1,0}, { 0,0,1},{ 0,0,-1} };

        for (auto& d : dirs) {
            int rx = x + d.dx, ry = y + d.dy, rz = z + d.dz;
            PieceColor opponent = (player_color == BLACK) ? WHITE : BLACK;

            while (rx >= 0 && rx < BOARD_SIZE && ry >= 0 && ry < BOARD_SIZE && rz >= 0 && rz < BOARD_SIZE &&
                   board_[rx][ry][rz] == opponent) {
                rx += d.dx; ry += d.dy; rz += d.dz;
            }

            if (rx >= 0 && rx < BOARD_SIZE && ry >= 0 && ry < BOARD_SIZE && rz >= 0 && rz < BOARD_SIZE &&
                board_[rx][ry][rz] == player_color) {
                int cx = x + d.dx, cy = y + d.dy, cz = z + d.dz;
                while (true) {
                    PieceColor c = board_[cx][cy][cz];
                    if (c == player_color || c == PieceColor::EMPTY) break;
                    result.flipped.push_back({cx,cy,cz});
                    board_[cx][cy][cz] = player_color;
                    cx += d.dx; cy += d.dy; cz += d.dz;
                }
            }
        }

        if (board_[x][y].at(z) == PieceColor::EMPTY) {
            result.success = true;
            board_[x][y][z] = player_color;
        }

        return result;
    }

    std::vector<std::array<int,3>> get_valid_moves_for_color(PieceColor color) const {
        std::vector<std::array<int,3>> valid;
        for (int x = 0; x < BOARD_SIZE; ++x) {
            for (int y = 0; y < BOARD_SIZE; ++y) {
                for (int z = 0; z < BOARD_SIZE; ++z) {
                    if (board_[x][y].at(z) == PieceColor::EMPTY) {
                        auto result = flip_stones(x, y, z, color);
                        if (result.success && !result.flipped.empty()) {
                            valid.push_back({x,y,z});
                        }
                    }
                }
            }
        }
        return valid;
    }

    int count_pieces(PieceColor color) const {
        int c = 0;
        for (int x = 0; x < BOARD_SIZE; ++x)
            for (int y = 0; y < BOARD_SIZE; ++y)
                for (int z = 0; z < BOARD_SIZE; ++z)
                    if (board_[x][y].at(z) == color) c++;
        return c;
    }

    bool is_full() const {
        for (int x = 0; x < BOARD_SIZE; ++x)
            for (int y = 0; y < BOARD_SIZE; ++y)
                if (board_[x][y].at(0) != PieceColor::EMPTY && board_[x][y][1] != PieceColor::EMPTY) return true;
        return false;
    }
};

// ============================================================================
class GameEngine {
private:
    CubeBoard board_;
    PieceColor turn_player_ = BLACK;  // 黒が先攻
    bool game_over_ = false;
    char winner_ = '.';

public:
    void reset() {
        board_.reset();   // 簡易：初期配置は GameEngine が管理しない（外部で board_.initialize_board() など）
        turn_player_ = BLACK;
        game_over_ = false;
        winner_ = '.';
    }

    const CubeBoard& get_board() const { return board_; }
    PieceColor get_turn_player() const { return turn_player_; }
    bool is_game_over() const { return game_over_; }
    char get_winner() const { return winner_; }

    std::vector<std::array<int,3>> get_valid_moves_for_current_player() {
        auto color = (turn_player_ == BLACK) ? PieceColor::BLACK : PieceColor::WHITE;
        return board_.get_valid_moves_for_color(color);
    }

    struct MoveResult {
        bool valid = false;
        int flipped_count = 0;
        const char* effect = nullptr;
        PieceColor placed_piece = PieceColor::EMPTY;
    };

    MoveResult play_move(int x, int y, int z) {
        if (game_over_) return {{false,0,nullptr,PieceColor::EMPTY}};

        auto result = board_.flip_stones(x, y, z, turn_player_);
        PieceColor placed_color = result.success ? turn_player_ : PieceColor::BLACK;  // デフォルト黒から開始

        MoveResult mr{result.success, static_cast<int>(result.flipped.size()), nullptr, placed_color};

        if (static_cast<int>(result.flipped.size()) >= 7) {
            mr.effect = "return_burst";   // ターンリセット（簡易版：このゲームでは実装しない）
        } else if (static_cast<int>(result.flipped.size()) >= 5) {
            mr.effect = "wall_strike";    // 壁設置（簡易版：無効）
        }

        return mr;
    }

    void switch_turn() {
        turn_player_ = (turn_player_ == BLACK ? WHITE : BLACK);
    }

    bool check_game_over() {
        if (board_.is_full()) {
            game_over_ = true;
            auto counts = count_pieces_internal();
            winner_ = (counts[BLACK] > counts[WHITE]) ? 'B' :
                       (counts[WHITE] > counts[BLACK]) ? 'W' : '.';
            return true;
        }

        auto black_moves = board_.get_valid_moves_for_color(PieceColor::BLACK);
        auto white_moves = board_.get_valid_moves_for_color(PieceColor::WHITE);

        if (black_moves.empty() && white_moves.empty()) {
            game_over_ = true;
            winner_ = '.';  // ドロー（パス）
            return true;
        }

        return false;
    }

    char get_winner_name() const {
        auto counts = count_pieces_internal();
        if (counts[BLACK] > counts[WHITE]) return 'B';
        if (counts[WHITE] > counts[BLACK]) return 'W';
        return '.';
    }

private:
    struct PieceCounts { int black; int white; };
    static inline PieceCounts count_pieces_internal() {
        PieceCounts c{0,0};
        for (int x = 0; x < BOARD_SIZE; ++x)
            for (int y = 0; y < BOARD_SIZE; ++y)
                for (int z = 0; z < BOARD_SIZE; ++z) {
                    if (board_[x][y].at(z) == PieceColor::BLACK) c.black++;
                    else if (board_[x][y].at(z) == PieceColor::WHITE) c.white++;
                }
        return c;
    }

    static inline PieceColor piece_color(char c) {
        switch(c) { case 'B': return PieceColor::BLACK; case 'W': return PieceColor::WHITE; default: return PieceColor::EMPTY; }
    }
};

} // namespace cubo
