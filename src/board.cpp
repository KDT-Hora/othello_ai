#include "board.hpp"
#include <array>
#include <vector>
#include <algorithm>

namespace cubo_othello {

void CubeBoard::initialize() {
    // Reset board to all empty
    for (int z = 0; z < Z_LAYERS; ++z) {
        for (int y = 0; y < BOARD_SIZE; ++y) {
            for (int x = 0; x < BOARD_SIZE; ++x) {
                grid_[x][y][z] = EMPTY;
            }
        }
    }

    // Place center stones at (x,y,z) ∈ {3,4}×{3,4}×{3,5}
    // Black: x∈{3}, White: x∈{4}; y∈{3,4}; z∈{3,4,5} → 8 pieces total

    for (int x : {3, 4}) {
        for (int y : {3, 4}) {
            for (int z : {3, 4, 5}) {
                if (x == 3) grid_[x][y][z] = Black;
                else        grid_[x][y][z] = White;
            }
        }
    }

    turn_   = BLACK;
    game_over_ = false;
}

int CubeBoard::place_stone(int x, int y, int z, int color) {
    if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE || z < 0 || z >= Z_LAYERS) return 0;

    if (grid_[x][y][z] != EMPTY) return 0;   // cell already occupied → invalid move

    int flipped_count = 0;
    const int opponent = (color == BLACK) ? WHITE : BLACK;

    // Scan all 6 axial directions: (+x,-x,+y,-y,+z,-z)
    constexpr auto dirs = std::array<std::pair<int,int>, 3>{
        {{+1, 0}, {-1, 0}},   // x-axis pair
        {{0, +1}, {0, -1}},   // y-axis pair
        {{0, 0},  {0, 0}}     // placeholder — will be filled below
    };

    for (int axis = 0; axis < 3; ++axis) {
        int dx = dirs[axis].first;
        int dy = dirs[axis].second;

        // Scan outward from the immediate neighbor in +direction
        int cx = x + dx, cy = y + dy, cz = z;   // start just beyond placed stone
        bool sandwich_found = false;
        std::vector<std::tuple<int,int,int>> flipped_cells;  // empty for now (we'll fill below)

        while (cx >= 0 && cx < BOARD_SIZE && cy >= 0 && cy < BOARD_SIZE && cz >= 0 && cz < Z_LAYERS) {
            if (grid_[cx][cy][cz] == color) {   // found own stone → sandwich complete!
                sandwich_found = true;
                break;
            } else if (grid_[cx][cy][cz] != EMPTY) {
                // opponent stone → keep scanning
                flipped_cells.emplace_back(cx, cy, cz);
            } else {
                // empty cell → stop scanning this direction
                break;
            }

            cx += dx; cy += dy; cz++;  // move outward
        }

        if (sandwich_found) {
            // Flip all collected opponent stones
            for (const auto& pos : flipped_cells) {
                grid_[std::get<0>(pos)][std::get<1>(pos)][std::get<2>(pos)] = color;
            }
            flipped_count += static_cast<int>(flipped_cells.size());
        }
    }

    // If no sandwich found in any direction, the move is invalid (Othello rules)
    if (!sandwich_found && flipped_count == 0) return 0;

    grid_[x][y][z] = color;   // place our stone
    game_over_ = false;        // game continues unless full or no-moves detected later
    return static_cast<int>(flipped_count);
}

bool CubeBoard::game_over() const {
    bool has_black_moves = get_valid_moves(BLACK).size() > 0;
    bool has_white_moves = get_valid_moves(WHITE).size() > 0;

    if (!has_black_moves && !has_white_moves) return true;   // no-moves condition

    bool board_full = true;
    for (int z = 0; z < Z_LAYERS; ++z) {
        for (int y = 0; y < BOARD_SIZE; ++y) {
            for (int x = 0; x < BOARD_SIZE; ++x) {
                if (grid_[x][y][z] == EMPTY) {
                    board_full = false;
                    break;
                }
            }
        }
    }

    if (!board_full) return true;   // full board → draw

    int black_count = count_pieces(BLACK);
    int white_count = count_pieces(WHITE);
    return (black_count == white_count);   // tie on full board → draw
}

std::vector<std::tuple<int,int,int>> CubeBoard::get_valid_moves(int color) const {
    std::vector<std::tuple<int,int,int>> valid;
    const int opponent = (color == BLACK) ? WHITE : BLACK;

    for (int z = 0; z < Z_LAYERS; ++z) {
        for (int y = 0; y < BOARD_SIZE; ++y) {
            for (int x = 0; x < BOARD_SIZE; ++x) {
                if (grid_[x][y][z] != EMPTY) continue;   // skip non-empty cells

                bool can_flip_any_direction = false;

                // Check all 6 directions for a sandwich opportunity
                constexpr auto dirs = std::array<std::pair<int,int>, 3>{
                    {{+1, 0}, {-1, 0}},
                    {{0, +1}, {0, -1}},
                    {{0, 0},  {0, 0}}   // z-axis placeholder
                };

                for (int axis = 0; axis < 3; ++axis) {
                    int dx = dirs[axis].first;
                    int dy = dirs[axis].second;
                    if (dx == 0 && dy == 0) continue;    // skip placeholder entry

                    int cx = x + dx, cy = y + dy, cz = z;
                    while (cx >= 0 && cx < BOARD_SIZE && cy >= 0 && cy < BOARD_SIZE && cz >= 0 && cz < Z_LAYERS) {
                        if (grid_[cx][cy][cz] == color) {   // found own stone → sandwich!
                            can_flip_any_direction = true;
                            break;
                        } else if (grid_[cx][cy][cz] != EMPTY) {
                            cx += dx; cy += dy; cz++;       // keep scanning
                        } else {
                            break;  // empty cell breaks the chain
                        }
                    }
                }

                if (can_flip_any_direction) {
                    valid.emplace_back(x, y, z);
                }
            }
        }
    }

    return valid;
}

int CubeBoard::count_pieces(int color) const {
    int count = 0;
    for (int z = 0; z < Z_LAYERS; ++z) {
        for (int y = 0; y < BOARD_SIZE; ++y) {
            for (int x = 0; x < BOARD_SIZE; ++x) {
                if (grid_[x][y][z] == color) count++;
            }
        }
    }
    return count;
}

} // namespace cubo_othello
