#pragma once

#include "libggg/graphs/parity_graph.hpp"
#include "libggg/solvers/solver.hpp"
#include <set>
#include <vector>

namespace ggg::solvers {

/**
 * @brief Buchi game solver using iterative attractor-based algorithm
 *
 * This solver implements a proven-correct algorithm for Buchi games.
 * For Buchi games, we use the parity graph structure but assume priorities are only 0 or 1.
 *
 * Algorithm:
 * - Player 0 wins if they can force a play that visits priority 1 vertices infinitely often
 * - We use an iterative approach:
 *   1. Find attractor of Player 1 to target vertices (priority 1)
 *   2. The complement becomes Player 0's target
 *   3. Find Player 0's attractor to this complement
 *   4. Remove Player 0's attractor from the game (mark as winning for Player 0)
 *   5. Repeat until all vertices are classified
 *
 * Time complexity: O(n * m) where n is vertices and m is edges
 */
class BuchiSolver : public Solver<graphs::ParityGraph, RSSolution<graphs::ParityGraph>> {
  public:
    /**
     * @brief Solve the Buchi game using iterative attractor algorithm
     * @param graph Parity graph representing Buchi game (priorities should be 0 or 1)
     * @return Solution with winning regions and strategy
     */
    RSSolution<graphs::ParityGraph> solve(const graphs::ParityGraph &graph) override;

    /**
     * @brief Get solver name
     * @return Solver description
     */
    std::string get_name() const override {
        return "Buchi Game Solver (Iterative Attractor Algorithm)";
    }

  private:
    /**
     * @brief Validate that the graph represents a valid Buchi game
     * @param graph The game graph to validate
     * @return True if valid Buchi game (priorities are 0 or 1)
     */
    bool validate_buchi_game(const graphs::ParityGraph &graph) const;

    /**
     * @brief Compute attractor set for a given player to a target set within active vertices
     * @param graph The game graph
     * @param active_vertices The currently active vertices in the subgame
     * @param curr_player The player whose attractor we compute
     * @param curr_target The target set for the attractor
     * @return Attractor set
     */
    std::set<graphs::ParityVertex> compute_attractor(const graphs::ParityGraph &graph,
                                                     const std::set<graphs::ParityVertex> &active_vertices,
                                                     int curr_player,
                                                     const std::set<graphs::ParityVertex> &curr_target);

    /**
     * @brief Get all vertices with priority 1 (Buchi accepting vertices)
     * @param graph The game graph
     * @return Set of vertices with priority 1
     */
    std::set<graphs::ParityVertex> get_buchi_accepting_vertices(const graphs::ParityGraph &graph) const;

    /**
     * @brief Compute the complement of a vertex set within active vertices
     * @param active_vertices The set of active vertices
     * @param vertices Vertex set to complement
     * @return Complement set (vertices in active_vertices but not in vertices)
     */
    std::set<graphs::ParityVertex> compute_complement(const std::set<graphs::ParityVertex> &active_vertices,
                                                      const std::set<graphs::ParityVertex> &inactive_vertices) const;

    // Statistic fields
    uint iterations;  // Total number of iterations for the game solution
    uint attractions; // Total number of vertex attractions for the game solution
};

} // namespace ggg::solvers
