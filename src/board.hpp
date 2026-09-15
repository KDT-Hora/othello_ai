#include <array>

#pragma once

namespace cubo_othello {

constexpr int BOARD_SIZE = 8;   // 8×8×8 cube
constexpr int Z_LAYERS = 8;     // depth layers (0..7)

enum class Color { EMPTY, BLACK, WHITE };
inline constexpr int Black = 0;
inline constexpr int White = 1;

struct CubeBoard {
    std::array<std::array<std::array<int8_t, BOARD_SIZE>, BOARD_SIZE>, Z_LAYERS> grid_;
    int turn_{};                // -1=unstarted, 0=BLACK, 1=WHITE
    bool game_over_{};

public:
    CubeBoard() = default;

    void initialize();
    int place_stone(int x, int y, int z, int color);
    std::vector<std::tuple<int,int,int>> get_valid_moves(int color) const;
    bool game_over() const;
    int count_pieces(int color) const;
};

} // namespace cubo_othello
