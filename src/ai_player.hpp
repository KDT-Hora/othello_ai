#pragma once

#include "board.hpp"
#include <optional>
#include <random>
#include <utility>

namespace cubo_othello {

// ─── Simple AI Agent (Minimax without alpha-beta pruning) ───────────────────────

struct SimpleAIPlayer {
    enum class Role { HUMAN, AI };

    struct Config {
        int search_depth = 3;      // minimax depth (plies: half-moves)
        bool use_random_tiebreak   = true;
        std::random_device rd{};
        std::mt19937 gen{rd()};
    };

    SimpleAIPlayer(Role role, Config config = {})
        : role(role), depth(config.search_depth), rng(config.gen) {}

    // Evaluate board from AI's perspective (positive = good for AI)
    int evaluate(const CubeBoard& board) const;

    // Search best move using minimax at given plies
    std::optional<std::tuple<int,int,int>> search(const CubeBoard&, bool is_maximizing_player) const;

private:
    Role role{};                    // HUMAN or AI
    int depth{};                   // remaining plies to search
    std::mt19937 rng{};            // for tie-breaking randomness

    // Minimax recursive helper (called internally by search())
    int minimax(const CubeBoard&, bool is_maximizing, int ply_left) const;

    // Helper: collect all valid moves for a given player
    std::vector<std::tuple<int,int,int>> get_legal_moves(const CubeBoard&, int color) const;
};

} // namespace cubo_othello
