// Cube Othello -- CubeBoard: 6x6x6 board state and rules.
#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace cubo_othello {

inline constexpr int BOARD_SIZE = 6;   // 6x6x6 cube (216 cells)

inline constexpr int8_t EMPTY = -1;
inline constexpr int8_t BLACK = 0;
inline constexpr int8_t WHITE = 1;

enum class GameOverReason { InProgress, Full, NoMoves };

struct Move {
    int x{};
    int y{};
    int z{};
};

// Pure board-state/rules class. Does not track whose turn it is --
// that is orchestrated by GameEngine, which keeps CubeBoard trivially
// testable and safe to copy for AI search.
class CubeBoard {
public:
    CubeBoard();

    // Reset to the 8-stone center opening (3D checkerboard, no same-color
    // neighbours) and clear all other cells.
    void initialize();

    // Empties every cell (no center stones). Test/setup helper.
    void clear();

    // Places `color` at (x,y,z) without validating Othello rules or
    // flipping anything. Test/setup helper for constructing arbitrary
    // positions; production code should use place_stone() instead.
    void set_for_testing(int x, int y, int z, int8_t color);

    int8_t at(int x, int y, int z) const;

    // Place `color` at (x,y,z), flipping any sandwiched opponent runs along
    // all 26 directions (axial and diagonal). Returns the number of stones flipped (not
    // counting the placed stone itself); 0 means the move was illegal and
    // the board is left unchanged.
    int place_stone(int x, int y, int z, int8_t color);

    // The opponent stones that placing `color` at (x,y,z) would flip (empty
    // if the move is illegal). Does not modify the board.
    std::vector<Move> flips_for(int x, int y, int z, int8_t color) const { return scan_flips(x, y, z, color); }

    // All empty cells from which `color` has at least one legal move.
    std::vector<Move> valid_moves(int8_t color) const;
    bool has_valid_moves(int8_t color) const;

    GameOverReason game_over_reason() const;

    int count(int8_t color) const;

    // Black stone count minus White stone count.
    int evaluate() const;

private:
    std::array<std::array<std::array<int8_t, BOARD_SIZE>, BOARD_SIZE>, BOARD_SIZE> grid_{};

    static bool in_bounds(int x, int y, int z);
    // Returns the list of opponent cells that would be flipped by placing
    // `color` at (x,y,z); empty if the move is illegal.
    std::vector<Move> scan_flips(int x, int y, int z, int8_t color) const;
};

} // namespace cubo_othello
