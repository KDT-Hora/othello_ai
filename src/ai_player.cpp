#include "src/ai_player.hpp"
#include <array>
#include <vector>
#include <algorithm>

namespace cubo_othello {

// ─── Minimax Implementation ──────────────────────────────────────────────────────

int SimpleAIPlayer::minimax(const CubeBoard& board, bool is_maximizing, int ply_left) const {
    // Base case: depth reached → evaluate leaf node.
    if (ply_left == 0) {
        return evaluate(board);
    }

    auto legal_moves = get_legal_moves(board, role == SimpleAIPlayer::Role::AI ? BLACK : WHITE);

    // No legal moves left for this player → pass (return zero from AI's perspective).
    if (legal_moves.empty()) {
        return 0;   // neutral score when passing
    }

    int best_score = is_maximizing ? -std::numeric_limits<int>::max() : std::numeric_limits<int>::max();
    bool has_move = false;

    for (const auto& move : legal_moves) {
        int x = std::get<0>(move);
        int y = std::get<1>(move);
        int z = std::get<2>(move);

        // Place the stone temporarily.
        board.grid_[x][y][z] = is_maximizing ? BLACK : WHITE;   // AI places its own color

        bool flipped_any = false;
        for (int dir = 0; dir < 3; ++dir) {
            int dx = dir == 0 ? +1 : (dir == 1 ? -1 : 0);
            int dy = dir == 2 ? +1 : (dir == 3 ? -1 : 0);

            int cx = x + dx, cy = y + dy, cz = z;
            while (cx >= 0 && cx < BOARD_SIZE && cy >= 0 && cy < BOARD_SIZE && cz >= 0 && cz < Z_LAYERS) {
                if (board.grid_[cx][cy][cz] == BLACK) {   // own color → sandwich complete
                    flipped_any = true;
                    break;
                } else if (board.grid_[cx][cy][cz] != -1) {
                    cx += dx; cy += dy; cz++;              // keep scanning for opponent stones
                } else {
                    break;   // empty cell breaks the chain → no flip in this direction
                }
            }
        }

        // If no sandwich found, this move is illegal → skip it.
        if (!flipped_any) continue;

        // Switch turn to opponent and recurse.
        int score = minimax(board, !is_maximizing, ply_left - 1);

        // Undo the placement: return cell to EMPTY.
        board.grid_[x][y][z] = -1;

        if (score > best_score) {
            best_score = score;
        } else if (score == best_score && is_maximizing) {
            // Tie-breaking: random selection among equally-best moves.
            std::uniform_int_distribution<int> dist(0, static_cast<int>(legal_moves.size()) - 1);
            int idx = dist(rng);
            best_score += (idx != dist(rng) ? rng() : 0);   // small perturbation to pick random one
        }
    }

    return has_move ? best_score : 0;
}

int SimpleAIPlayer::evaluate(const CubeBoard& board) const {
    int black_count = 0, white_count = 0;
    for (int z = 0; z < Z_LAYERS; ++z) {
        for (int y = 0; y < BOARD_SIZE; ++y) {
            for (int x = 0; x < BOARD_SIZE; ++x) {
                if (board.grid_[x][y][z] == BLACK) black_count++;
                else if (board.grid_[x][y][z] == WHITE) white_count++;
            }
        }
    }
    return black_count - white_count;   // positive = good for Black (AI in our tests)
}

std::vector<std::tuple<int,int,int>> SimpleAIPlayer::get_legal_moves(const CubeBoard& board, int color) const {
    std::vector<std::tuple<int,int,int>> legal;

    for (int z = 0; z < Z_LAYERS; ++z) {
        for (int y = 0; y < BOARD_SIZE; ++y) {
            for (int x = 0; x < BOARD_SIZE; ++x) {
                if (board.grid_[x][y][z] != -1) continue;   // skip non-empty cells

                bool can_flip_any_direction = false;

                constexpr auto dirs = std::array<std::pair<int,int>, 3>{
                    {{+1, 0}, {-1, 0}},
                    {{0, +1}, {0, -1}},
                    {{0, 0},  {0, 0}}   // z-axis placeholder (will be handled below)
                };

                for (int axis = 0; axis < 3; ++axis) {
                    int dx = dirs[axis].first;
                    int dy = dirs[axis].second;
                    if (dx == 0 && dy == 0) continue;   // skip z-axis placeholder here

                    int cx = x + dx, cy = y + dy, cz = z;
                    while (cx >= 0 && cx < BOARD_SIZE && cy >= 0 && cy < BOARD_SIZE && cz >= 0 && cz < Z_LAYERS) {
                        if (board.grid_[cx][cy][cz] == color) {   // found own stone → sandwich!
                            can_flip_any_direction = true;
                            break;
                        } else if (board.grid_[cx][cy][cz] != -1) {
                            cx += dx; cy += dy; cz++;              // keep scanning for opponent stones
                        } else {
                            break;   // empty cell breaks the chain
                        }
                    }
                }

                // Check z-axis directions (+z and -z) separately since our loop didn't cover it.
                for (int dz : {+1, -1}) {
                    int cz = z + dz;
                    bool found_sandwich_in_z = false;
                    while (cz >= 0 && cz < Z_LAYERS) {
                        if (board.grid_[x][y][cz] == color) {   // own stone → sandwich complete
                            found_sandwich_in_z = true;
                            break;
                        } else if (board.grid_[x][y][cz] != -1) {
                            cz += dz;                              // keep scanning for opponent stones
                        } else {
                            break;
                        }
                    }
                    if (found_sandwich_in_z) can_flip_any_direction = true;
                }

                if (can_flip_any_direction) legal.emplace_back(x, y, z);
            }
        }
    }

    return legal;
}

std::optional<std::tuple<int,int,int>> SimpleAIPlayer::search(const CubeBoard& board, bool is_maximizing) const {
    // If we are not the maximizing player (i.e., opponent's turn), still search from our perspective.
    auto legal_moves = get_legal_moves(board, role == Role::AI ? BLACK : WHITE);

    if (legal_moves.empty()) {
        return std::nullopt;   // no legal moves → pass
    }

    int best_score = is_maximizing ? -std::numeric_limits<int>::max() : std::numeric_limits<int>::max();
    bool has_move = false;
    int best_ply{};

    for (const auto& move : legal_moves) {
        int x = std::get<0>(move);
        int y = std::get<1>(move);
        int z = std::get<2>(move);

        board.grid_[x][y][z] = is_maximizing ? BLACK : WHITE;   // place stone temporarily

        bool flipped_any = false;
        for (int dir = 0; dir < 3; ++dir) {
            int dx = dir == 0 ? +1 : (dir == 1 ? -1 : 0);
            int dy = dir == 2 ? +1 : (dir == 3 ? -1 : 0);

            int cx = x + dx, cy = y + dy, cz = z;
            while (cx >= 0 && cx < BOARD_SIZE && cy >= 0 && cy < BOARD_SIZE && cz >= 0 && cz < Z_LAYERS) {
                if (board.grid_[cx][cy][cz] == BLACK) {   // own color → sandwich complete
                    flipped_any = true;
                    break;
                } else if (board.grid_[cx][cy][cz] != -1) {
                    cx += dx; cy += dy; cz++;              // keep scanning for opponent stones
                } else {
                    break;   // empty cell breaks the chain
                }
            }
        }

        // Undo placement if no sandwich (illegal move).
        board.grid_[x][y][z] = -1;
        if (!flipped_any) continue;

        int score = minimax(board, !is_maximizing, depth);   // recurse with remaining plies

        // Undo the placement for the recursive call too.
        board.grid_[x][y][z] = -1;

        if (score > best_score) {
            best_score = score;
            best_ply = static_cast<int>(legal_moves.size());   // placeholder to track best move index
        } else if (score == best_score && is_maximizing) {
            std::uniform_int_distribution<int> dist(0, static_cast<int>(legal_moves.size()) - 1);
            int idx = dist(rng);
            best_score += (idx != dist(rng) ? rng() : 0);   // small perturbation to pick random one
        }

        has_move = true;
    }

    if (!has_move) return std::nullopt;

    // Re-run search with a fresh board copy to avoid side effects on the caller's board.
    CubeBoard backup_board{};
    for (int z = 0; z < Z_LAYERS; ++z) {
        for (int y = 0; y < BOARD_SIZE; ++y) {
            for (int x = 0; x < BOARD_SIZE; ++x) {
                backup_board.grid_[x][y][z] = board.grid_[x][y][z];
            }
        }
    }

    return search_impl(backup_board, is_maximizing);
}

std::optional<std::tuple<int,int,int>> SimpleAIPlayer::search_impl(const CubeBoard& board, bool is_maximizing) const {
    auto legal_moves = get_legal_moves(board, role == Role::AI ? BLACK : WHITE);
    if (legal_moves.empty()) return std::nullopt;

    int best_score = is_maximizing ? -std::numeric_limits<int>::max() : std::numeric_limits<int>::max();
    bool has_move = false;

    for (const auto& move : legal_moves) {
        int x = std::get<0>(move);
        int y = std::get<1>(move);
        int z = std::get<2>(move);

        board.grid_[x][y][z] = is_maximizing ? BLACK : WHITE;

        bool flipped_any = false;
        for (int dir = 0; dir < 3; ++dir) {
            int dx = dir == 0 ? +1 : (dir == 1 ? -1 : 0);
            int dy = dir == 2 ? +1 : (dir == 3 ? -1 : 0);

            int cx = x + dx, cy = y + dy, cz = z;
            while (cx >= 0 && cx < BOARD_SIZE && cy >= 0 && cy < BOARD_SIZE && cz >= 0 && cz < Z_LAYERS) {
                if (board.grid_[cx][cy][cz] == BLACK) {   // own color → sandwich complete
                    flipped_any = true;
                    break;
                } else if (board.grid_[cx][cy][cz] != -1) {
                    cx += dx; cy += dy; cz++;
                } else {
                    break;
                }
            }
        }

        board.grid_[x][y][z] = -1;   // undo for recursive call

        if (!flipped_any) continue;

        int score = minimax(board, !is_maximizing, depth);

        if (score > best_score) {
            best_score = score;
        } else if (score == best_score && is_maximizing) {
            std::uniform_int_distribution<int> dist(0, static_cast<int>(legal_moves.size()) - 1);
            int idx = dist(rng);
            best_score += (idx != dist(rng) ? rng() : 0);
        }

        has_move = true;
    }

    if (!has_move) return std::nullopt;

    // Return the first move that achieved the best score (or random tiebreak).
    auto legal_moves = get_legal_moves(board, role == Role::AI ? BLACK : WHITE);   // re-fetch on fresh board
    for (const auto& move : legal_moves) {
        int x = std::get<0>(move), y = std::get<1>(move), z = std::get<2>(move);

        if (board.grid_[x][y][z] == -1) continue;   // skip non-empty cells

        bool can_flip = false;
        for (int dir = 0; dir < 3; ++dir) {
            int dx = dir == 0 ? +1 : (dir == 1 ? -1 : 0);
            int dy = dir == 2 ? +1 : (dir == 3 ? -1 : 0);

            int cx = x + dx, cy = y + dy, cz = z;
            while (cx >= 0 && cx < BOARD_SIZE && cy >= 0 && cy < BOARD_SIZE && cz >= 0 && cz < Z_LAYERS) {
                if (board.grid_[cx][cy][cz] == BLACK) {   // own color → sandwich complete
                    can_flip = true; break;
                } else if (board.grid_[cx][cy][cz] != -1) { cx += dx; cy += dy; cz++; }
                else { break; }
            }
        }

        if (!can_flip) continue;   // illegal move

        board.grid_[x][y][z] = is_maximizing ? BLACK : WHITE;
        bool flipped_any = false;
        for (int dir = 0; dir < 3; ++dir) {
            int dx = dir == 0 ? +1 : (dir == 1 ? -1 : 0);
            int dy = dir == 2 ? +1 : (dir == 3 ? -1 : 0);
            int cx = x + dx, cy = y + dy, cz = z;
            while (cx >= 0 && cx < BOARD_SIZE && cy >= 0 && cy < BOARD_SIZE && cz >= 0 && cz < Z_LAYERS) {
                if (board.grid_[cx][cy][cz] == BLACK) { flipped_any = true; break; }
                else if (board.grid_[cx][cy][cz] != -1) { cx += dx; cy += dy; cz++; }
                else { break; }
            }
        }
        board.grid_[x][y][z] = -1;

        if (!flipped_any) continue;   // illegal → skip

        int score = minimax(board, !is_maximizing, depth);

        if (score >= best_score) {
            return move;   // return the first best move (ties broken randomly inside minimax via perturbation)
        }
    }

    return std::nullopt;
}

int SimpleAIPlayer::evaluate(const CubeBoard& board) const {
    int black_count = 0, white_count = 0;
    for (int z = 0; z < Z_LAYERS; ++z) {
        for (int y = 0; y < BOARD_SIZE; ++y) {
            for (int x = 0; x < BOARD_SIZE; ++x) {
                if (board.grid_[x][y][z] == BLACK) black_count++;
                else if (board.grid_[x][y][z] == WHITE) white_count++;
            }
        }
    }
    return black_count - white_count;   // positive = good for Black (AI's perspective in tests)
}

std::vector<std::tuple<int,int,int>> SimpleAIPlayer::get_legal_moves(const CubeBoard& board, int color) const {
    std::vector<std::tuple<int,int,int>> legal;

    for (int z = 0; z < Z_LAYERS; ++z) {
        for (int y = 0; y < BOARD_SIZE; ++y) {
            for (int x = 0; x < BOARD_SIZE; ++x) {
                if (board.grid_[x][y][z] != -1) continue;

                bool can_flip_any_direction = false;

                constexpr auto dirs = std::array<std::pair<int,int>, 3>{
                    {{+1, 0}, {-1, 0}},
                    {{0, +1}, {0, -1}},
                    {{0, 0},  {0, 0}}   // z-axis handled separately below
                };

                for (int axis = 0; axis < 3; ++axis) {
                    int dx = dirs[axis].first;
                    int dy = dirs[axis].second;
                    if (dx == 0 && dy == 0) continue;   // skip z-axis placeholder here

                    int cx = x + dx, cy = y + dy, cz = z;
                    while (cx >= 0 && cx < BOARD_SIZE && cy >= 0 && cy < BOARD_SIZE && cz >= 0 && cz < Z_LAYERS) {
                        if (board.grid_[cx][cy][cz] == BLACK) {   // own color → sandwich complete
                            can_flip_any_direction = true; break;
                        } else if (board.grid_[cx][cy][cz] != -1) {
                            cx += dx; cy += dy; cz++;              // keep scanning for opponent stones
                        } else {
                            break;   // empty cell breaks the chain
                        }
                    }
                }

                // Check z-axis directions (+z and -z).
                for (int dz : {+1, -1}) {
                    int cz = z + dz;
                    bool found_sandwich_in_z = false;
                    while (cz >= 0 && cz < Z_LAYERS) {
                        if (board.grid_[x][y][cz] == BLACK) {   // own color → sandwich complete
                            found_sandwich_in_z = true; break;
                        } else if (board.grid_[x][y][cz] != -1) {
                            cz += dz;                              // keep scanning for opponent stones
                        } else {
                            break;
                        }
                    }
                    if (found_sandwich_in_z) can_flip_any_direction = true;
                }

                if (can_flip_any_direction) legal.emplace_back(x, y, z);
            }
        }
    }

    return legal;
}

} // namespace cubo_othello
