// Cube Othello — Game Engine (C++ / DxLib)
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

        // 初期配置: (3,3)-(4,4) の中心部で黒・白が隣り合うように配置
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

    void reset() override {
        for (int x = 0; x < BOARD_SIZE; ++x)
            for (int y = 0; y < BOARD_SIZE; ++y)
                board_[x][y].fill(PieceColor::EMPTY);
    }

    char get_cell(int x, int y, int z) const {
        if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE ||
            z < 0 || z >= BOARD_SIZE) return '.';
        switch (board_[x][y][z]) {
            case PieceColor::EMPTY: return '.';
            case PieceColor::BLACK: return 'B';
            case PieceColor::WHITE: return 'W';
            default:                 return '?';
        }
    }

    bool set_cell(int x, int y, int z, PieceColor color) {
        if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE ||
            z < 0 || z >= BOARD_SIZE) return false;
        if (board_[x][y].at(z) != PieceColor::EMPTY) return false;
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
            PieceColor opponent = (player_color == BLACK) ? WHITE : WHITE;

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
                        if (!result.flipped.empty()) valid.push_back({x,y,z});
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
    PieceColor turn_player_ = BLACK;
    bool game_over_ = false;
    char winner_ = '.';

public:
    void reset() { board_.reset(); turn_player_ = BLACK; game_over_ = false; winner_ = '.'; }

    const CubeBoard& get_board() const { return board_; }
    PieceColor get_turn_player() const { return turn_player_; }
    bool is_game_over() const { return game_over_; }
    char get_winner() const { return winner_; }

    std::vector<std::array<int,3>> get_valid_moves_for_current_player() {
        auto color = (turn_player_ == BLACK) ? PieceColor::BLACK : PieceColor::WHITE;
        return board_.get_valid_moves_for_color(color);
    }

    struct MoveResult { bool valid; int flipped_count; const char* effect; };
    MoveResult play_move(int x, int y, int z) {
        if (game_over_) return {{false,0,nullptr}};
        auto result = board_.flip_stones(x, y, z, turn_player_);
        MoveResult mr{!result.flipped.empty(), static_cast<int>(result.flipped.size()), nullptr};

        if (static_cast<int>(result.flipped.size()) >= 7) {
            mr.effect = "return_burst";
            // ターンリセットは簡易版では実装しない
        } else if (static_cast<int>(result.flipped.size()) >= 5) {
            mr.effect = "wall_strike";
        }

        return mr;
    }

    void switch_turn() { turn_player_ = (turn_player_ == BLACK ? WHITE : BLACK); }

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
};

// ============================================================================
class SimpleAI {
public:
    int depth = 3;

    enum class MoveType { PLACE, PASS };

    struct AIResult { MoveType type; std::array<int,3> move; };

    AIResult search(const CubeBoard& board, PieceColor ai_color) const {
        auto valid = board.get_valid_moves_for_color(ai_color);
        if (valid.empty()) return {{MoveType::PASS, {-1,-1,-1}}};

        int best_score = -9999;
        AIResult best{MoveType::PLACE, {-1,-1,-1}};

        for (auto& move : valid) {
            auto flipped = board.flip_stones(move[0], move[1], move[2], ai_color);

            // 簡易評価：ひっくり返る石数 × 重み + 最終盤面の石数差
            int score = static_cast<int>(flipped.size()) * 3;

            if (board.count_pieces(ai_color) > board.count_pieces(PieceColor::WHITE)) {
                score += 10;
            } else if (board.count_pieces(ai_color) < board.count_pieces(PieceColor::WHITE)) {
                score -= 5;
            }

            // シンプルな評価だが、挟み込み枚数のみを重視した簡易 AI
            if (score > best_score) {
                best_score = score;
                best.move = move;
                best.type = MoveType::PLACE;
            }
        }

        return best;
    }
};

} // namespace cubo
