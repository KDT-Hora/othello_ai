// Cube Othello -- AI search equivalence harness (manual run, not part of ctest).
//
// Verifies that the alpha-beta SimpleAI is behaviourally identical to the
// original unpruned brute-force search: on many realistic positions (boards
// produced by random legal play from the opening), the chosen move must always
// lie in the exact optimal set computed by the reference algorithm, choose_move
// must never mutate the caller's board, and apply_move/undo_move must round-
// trip exactly. Also times old vs new search on a realistic late-midgame board.
//
// Build: part of the cubo_ai_equiv_harness CMake target (no DXLib needed).
// Run:   build\Debug\cubo_ai_equiv_harness.exe
#include "ai_player.hpp"
#include "board.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <limits>
#include <random>
#include <vector>

using namespace cubo_othello;

namespace {

int8_t opp(int8_t c) { return c == BLACK ? WHITE : BLACK; }

// --- Reference implementation: the original unpruned algorithm verbatim ---
long long g_nodes = 0;

int ref_minimax(CubeBoard board, int8_t to_move, int8_t ai_color, int ply_left) {
    ++g_nodes;
    if (ply_left <= 0) return board.count(ai_color) - board.count(opp(ai_color));

    auto moves = board.valid_moves(to_move);
    if (moves.empty()) {
        const int8_t o = opp(to_move);
        if (!board.has_valid_moves(o)) return board.count(ai_color) - board.count(opp(ai_color));
        return ref_minimax(board, o, ai_color, ply_left - 1);
    }

    const bool maximizing = (to_move == ai_color);
    int best = maximizing ? std::numeric_limits<int>::min() : std::numeric_limits<int>::max();
    for (const auto& mv : moves) {
        CubeBoard next = board;
        next.place_stone(mv.x, mv.y, mv.z, to_move);
        int score = ref_minimax(next, opp(to_move), ai_color, ply_left - 1);
        best = maximizing ? std::max(best, score) : std::min(best, score);
    }
    return best;
}

std::vector<std::pair<Move, int>> ref_root_values(const CubeBoard& board, int8_t ai_color, int depth) {
    std::vector<std::pair<Move, int>> out;
    for (const auto& mv : board.valid_moves(ai_color)) {
        CubeBoard next = board;
        next.place_stone(mv.x, mv.y, mv.z, ai_color);
        out.push_back({mv, ref_minimax(next, opp(ai_color), ai_color, depth - 1)});
    }
    return out;
}

bool same_move(const Move& a, const Move& b) { return a.x == b.x && a.y == b.y && a.z == b.z; }

// Realistic position: random legal play from the standard opening until roughly
// `moves_to_play` placements have been made. Produces the branching factors a
// real game actually has, unlike random stone scatter.
CubeBoard play_random_game(std::mt19937& rng, int moves_to_play) {
    CubeBoard board; // standard opening
    int8_t turn = BLACK;
    for (int i = 0; i < moves_to_play; ++i) {
        auto lm = board.legal_moves_with_flips(turn);
        if (lm.empty()) {
            const int8_t o = opp(turn);
            if (board.legal_moves_with_flips(o).empty()) break; // game over early
            turn = o;
            continue;
        }
        std::uniform_int_distribution<size_t> pick(0, lm.size() - 1);
        const auto& m = lm[pick(rng)];
        board.apply_move(m.move.x, m.move.y, m.move.z, turn);
        turn = opp(turn);
    }
    return board;
}

bool boards_equal(const CubeBoard& a, const CubeBoard& b) {
    for (int x = 0; x < BOARD_SIZE; ++x)
        for (int y = 0; y < BOARD_SIZE; ++y)
            for (int z = 0; z < BOARD_SIZE; ++z)
                if (a.at(x, y, z) != b.at(x, y, z)) return false;
    return true;
}

} // namespace

int main() {
    int checked = 0, failures = 0;
    long long ref_nodes = 0;

    // --- Correctness on realistic positions: chosen move must be in the
    //     brute-force optimal set, at depths 1..3, for both colors. ---
    for (int trial = 0; trial < 24; ++trial) {
        std::mt19937 rng(1000u + static_cast<unsigned>(trial));
        const int moves_to_play = 6 + trial * 2; // 6..52 placements: opening → late midgame
        CubeBoard board = play_random_game(rng, moves_to_play);

        for (const int8_t color : {BLACK, WHITE}) {
            const int depth = 1 + trial % 3; // 1..3
            if (board.valid_moves(color).empty()) continue;

            g_nodes = 0;
            auto roots = ref_root_values(board, color, depth);
            ref_nodes += g_nodes;
            if (roots.empty()) continue;

            int best_val = std::numeric_limits<int>::min();
            for (auto& r : roots) best_val = std::max(best_val, r.second);

            SimpleAI ai(color, depth);
            const CubeBoard before = board;
            auto chosen = ai.choose_move(board);

            ++checked;
            if (!chosen) {
                std::printf("FAIL trial %d color %d: nullopt despite %zu legal moves\n",
                            trial, (int)color, roots.size());
                ++failures;
                continue;
            }
            bool in_optimal = false;
            for (auto& r : roots)
                if (r.second == best_val && same_move(r.first, *chosen)) in_optimal = true;
            if (!in_optimal) {
                std::printf("FAIL trial %d color %d depth %d: chose (%d,%d,%d) outside optimal set (best=%d)\n",
                            trial, (int)color, depth, chosen->x, chosen->y, chosen->z, best_val);
                ++failures;
            }
            if (!boards_equal(board, before)) {
                std::printf("FAIL trial %d: choose_move mutated the caller's board\n", trial);
                ++failures;
            }
        }
    }
    std::printf("correctness: %d (position,color,depth) cases checked, %d failures "
                "(reference nodes explored: %lld)\n", checked, failures, ref_nodes);

    // --- apply/undo round-trip on realistic legal moves. ---
    int undo_failures = 0;
    for (int trial = 0; trial < 24; ++trial) {
        std::mt19937 rng(5000u + static_cast<unsigned>(trial));
        CubeBoard board = play_random_game(rng, 30);
        for (const int8_t color : {BLACK, WHITE}) {
            auto lm = board.legal_moves_with_flips(color);
            if (lm.empty()) continue;
            const CubeBoard snapshot = board;
            const auto& m = lm[trial % lm.size()];
            board.apply_move(m.move.x, m.move.y, m.move.z, color);
            board.undo_move(m.move, m.flips, color);
            if (!boards_equal(board, snapshot)) {
                std::printf("FAIL undo trial %d color %d\n", trial, (int)color);
                ++undo_failures;
            }
        }
    }
    std::printf("apply/undo round-trip: %d failures\n", undo_failures);

    // --- Speed on a realistic late-midgame board, depth 3, old vs new. ---
    {
        std::mt19937 rng(4242u);
        CubeBoard board = play_random_game(rng, 60);
        const size_t root_moves = board.valid_moves(BLACK).size();
        SimpleAI ai(BLACK, 3);

        auto t0 = std::chrono::steady_clock::now();
        auto new_move = ai.choose_move(board);
        auto t1 = std::chrono::steady_clock::now();

        g_nodes = 0;
        (void)ref_root_values(board, BLACK, 3); // old algorithm, same position/depth
        auto t2 = std::chrono::steady_clock::now();

        double new_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        double old_ms = std::chrono::duration<double, std::milli>(t2 - t1).count();
        std::printf("midgame depth-3 timing: new(alpha-beta) %.1f ms vs old(full-width) %.1f ms "
                    "(%.1fx speedup; %lld reference nodes; legal root moves=%zu; stones B/W=%d/%d)\n",
                    new_ms, old_ms, old_ms / std::max(0.001, new_ms), g_nodes, root_moves,
                    board.count(BLACK), board.count(WHITE));
        if (!new_move && root_moves > 0) {
            std::printf("FAIL: new AI passed despite legal moves\n");
            ++failures;
        }
    }

    std::printf(failures == 0 && undo_failures == 0 ? "OVERALL: PASS\n" : "OVERALL: FAIL\n");
    return failures == 0 && undo_failures == 0 ? 0 : 1;
}
