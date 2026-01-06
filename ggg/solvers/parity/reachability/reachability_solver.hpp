#pragma once

#include "libggg/graphs/parity_graph.hpp"
#include "libggg/solvers/solver.hpp"
#include <set>

namespace ggg {
namespace solvers {

/**
 * @brief Reachability game solver using attractor computation
 * @complexity Time: O(n + m)
 *
 * This solver implements an attractor-based algorithm for reachability games.
 * For reachability games, we use the parity graph structure where priorities indicate target vertices:
 * - Priority 1: Target vertices (Player 0 wants to reach these)
 * - Priority 0: Non-target vertices
 *
 * Algorithm:
 * - Player 0 wins if they can force a play that reaches a target vertex (priority 1)
 * - We compute the attractor set for Player 0 to all target vertices
 * - Vertices in the attractor are won by Player 0, all others by Player 1
 *
 */
class ReachabilitySolver : public Solver<graphs::ParityGraph, RSSolution<graphs::ParityGraph>> {
  public:
    /**
     * @brief Solve the reachability game using attractor computation
     * @param graph Parity graph representing reachability game (priority 1 = target, priority 0 = non-target)
     * @return Solution with winning regions and strategy
     */
    RSSolution<graphs::ParityGraph> solve(const graphs::ParityGraph &graph) override;

    /**
     * @brief Get solver name
     * @return Solver description
     */
    std::string get_name() const override {
        return "Reachability Game Solver (Attractor Algorithm)";
    }

  private:
    /**
     * @brief Validate that the graph represents a valid reachability game
     * @param graph The game graph to validate
     * @return True if valid reachability game (priorities are 0 or 1)
     */
    bool validate_reachability_game(const graphs::ParityGraph &graph) const;

    /**
     * @brief Compute attractor set for a player to a target set
     * @param graph The game graph
     * @param target Target vertex set
     * @param player Player computing attractor (0 or 1)
     * @return Attractor set and strategy mapping
     */
    std::pair<std::set<graphs::ParityVertex>, std::map<graphs::ParityVertex, graphs::ParityVertex>>
    compute_attractor(const graphs::ParityGraph &graph,
                      const std::set<graphs::ParityVertex> &target,
                      int player) const;

    /**
     * @brief Get all target vertices (vertices with priority 1)
     * @param graph The game graph
     * @return Set of vertices with priority 1 (target vertices)
     */
    std::set<graphs::ParityVertex> get_target_vertices(const graphs::ParityGraph &graph) const;
};

} // namespace solvers
} // namespace ggg
