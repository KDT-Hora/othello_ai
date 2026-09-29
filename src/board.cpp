#include "board.hpp"

namespace cubo_othello {

namespace {

// The 6 axial directions: +x,-x,+y,-y,+z,-z.
constexpr std::array<std::array<int, 3>, 6> kDirections{{
    {1, 0, 0}, {-1, 0, 0},
    {0, 1, 0}, {0, -1, 0},
    {0, 0, 1}, {0, 0, -1},
}};

} // namespace

CubeBoard::CubeBoard() { initialize(); }

void CubeBoard::clear() {
    for (auto& plane : grid_)
        for (auto& row : plane)
            row.fill(EMPTY);
}

void CubeBoard::set_for_testing(int x, int y, int z, int8_t color) { grid_[x][y][z] = color; }

void CubeBoard::initialize() {
    clear();

    // Center 2x2x2 in a 3D checkerboard: no two same-colored stones are
    // face-adjacent, and each color gets 4 stones.
    for (int x : {3, 4})
        for (int y : {3, 4})
            for (int z : {3, 4})
                grid_[x][y][z] = ((x + y + z) % 2 == 0) ? BLACK : WHITE;
}

bool CubeBoard::in_bounds(int x, int y, int z) {
    return x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE && z >= 0 && z < BOARD_SIZE;
}

int8_t CubeBoard::at(int x, int y, int z) const { return grid_[x][y][z]; }

std::vector<Move> CubeBoard::scan_flips(int x, int y, int z, int8_t color) const {
    if (!in_bounds(x, y, z) || grid_[x][y][z] != EMPTY) return {};

    const int8_t opponent = (color == BLACK) ? WHITE : BLACK;
    std::vector<Move> flips;

    for (const auto& dir : kDirections) {
        std::vector<Move> run;
        int cx = x + dir[0], cy = y + dir[1], cz = z + dir[2];

        while (in_bounds(cx, cy, cz) && grid_[cx][cy][cz] == opponent) {
            run.push_back({cx, cy, cz});
            cx += dir[0]; cy += dir[1]; cz += dir[2];
        }

        if (!run.empty() && in_bounds(cx, cy, cz) && grid_[cx][cy][cz] == color) {
            flips.insert(flips.end(), run.begin(), run.end());
        }
    }

    return flips;
}

int CubeBoard::place_stone(int x, int y, int z, int8_t color) {
    auto flips = scan_flips(x, y, z, color);
    if (flips.empty()) return 0;

    grid_[x][y][z] = color;
    for (const auto& m : flips) grid_[m.x][m.y][m.z] = color;

    return static_cast<int>(flips.size());
}

std::vector<Move> CubeBoard::valid_moves(int8_t color) const {
    std::vector<Move> moves;
    for (int x = 0; x < BOARD_SIZE; ++x) {
        for (int y = 0; y < BOARD_SIZE; ++y) {
            for (int z = 0; z < BOARD_SIZE; ++z) {
                if (grid_[x][y][z] != EMPTY) continue;
                if (!scan_flips(x, y, z, color).empty()) moves.push_back({x, y, z});
            }
        }
    }
    return moves;
}

bool CubeBoard::has_valid_moves(int8_t color) const {
    for (int x = 0; x < BOARD_SIZE; ++x)
        for (int y = 0; y < BOARD_SIZE; ++y)
            for (int z = 0; z < BOARD_SIZE; ++z)
                if (grid_[x][y][z] == EMPTY && !scan_flips(x, y, z, color).empty())
                    return true;
    return false;
}

int CubeBoard::count(int8_t color) const {
    int n = 0;
    for (const auto& plane : grid_)
        for (const auto& row : plane)
            for (int8_t cell : row)
                if (cell == color) ++n;
    return n;
}

int CubeBoard::evaluate() const { return count(BLACK) - count(WHITE); }

GameOverReason CubeBoard::game_over_reason() const {
    bool any_empty = false;
    for (const auto& plane : grid_) {
        for (const auto& row : plane) {
            for (int8_t cell : row) {
                if (cell == EMPTY) { any_empty = true; break; }
            }
            if (any_empty) break;
        }
        if (any_empty) break;
    }
    if (!any_empty) return GameOverReason::Full;

    if (!has_valid_moves(BLACK) && !has_valid_moves(WHITE)) return GameOverReason::NoMoves;

    return GameOverReason::InProgress;
}

} // namespace cubo_othello
