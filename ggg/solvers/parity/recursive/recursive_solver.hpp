#pragma once

#include "libggg/graphs/parity_graph.hpp"
#include "libggg/solvers/solver.hpp"
#include <boost/graph/graph_traits.hpp>
#include <map>
#include <set>

namespace ggg {
namespace solvers {

/**
 * @brief Solution type for recursive solver that includes statistics
 */
class RecursiveParitySolution : public RSSolution<graphs::ParityGraph> {
  private:
    size_t max_depth_reached_ = 0;
    size_t subgames_created_ = 0;

  public:
    RecursiveParitySolution() = default;
    explicit RecursiveParitySolution(bool solved, bool valid = true) : RSSolution<graphs::ParityGraph>(solved, valid) {}

    /**
     * @brief Set maximum recursion depth reached
     */
    void set_max_depth_reached(size_t depth) { max_depth_reached_ = depth; }

    /**
     * @brief Set number of subgames created
     */
    void set_subgames_created(size_t count) { subgames_created_ = count; }

    /**
     * @brief Get statistics as a generic map of key-value pairs
     * @return Map of statistic names to string values
     */
    std::map<std::string, std::string> get_statistics() const {
        std::map<std::string, std::string> stats;
        stats["max_depth_reached"] = std::to_string(max_depth_reached_);
        stats["subgames_created"] = std::to_string(subgames_created_);
        return stats;
    }

    /**
     * @brief Get maximum recursion depth reached (for backward compatibility)
     */
    size_t get_max_depth_reached() const { return max_depth_reached_; }

    /**
     * @brief Get number of subgames created (for backward compatibility)
     */
    size_t get_subgames_created() const { return subgames_created_; }
};

/**
 * @brief Simple recursive parity game solver
 *
 * This is a basic implementation of a recursive parity game solving algorithm.
 * It's not the most efficient but serves as a good demonstration and baseline.
 * The algorithm works by:
 * 1. Finding the highest priority in the game
 * 2. Computing the attractor set for that priority's player
 * 3. Recursively solving the remaining subgame
 * 4. Propagating results back
 * @complexity Time: O(2^n), Space: O(nc) where n = vertices and c = max colour
 */
class RecursiveParitySolver : public Solver<graphs::ParityGraph, RecursiveParitySolution> {
  public:
    /**
     * @brief Constructor that initializes solver state
     */
    RecursiveParitySolver();

    /**
     * @brief Constructor with recursion depth limit
     * @param max_depth Maximum recursion depth (0 for unlimited)
     */
    explicit RecursiveParitySolver(size_t max_depth);
    /**
     * @brief Solve the parity game recursively
     * @param graph Parity graph to solve
     * @return Solution with winning regions and strategies
     */
    RecursiveParitySolution solve(const graphs::ParityGraph &graph) override;

    /**
     * @brief Get solver name
     * @return Solver description
     */
    std::string get_name() const override {
        return "Recursive Parity Game Solver";
    }

    /**
     * @brief Get current recursion depth (for debugging/analysis)
     */
    size_t get_current_depth() const { return current_depth_; }

  private:
    // Configuration members initialized in constructor
    size_t max_recursion_depth_; ///< Maximum allowed recursion depth (0 = unlimited)
    bool enable_statistics_;     ///< Whether to collect solver statistics

    // Runtime state members (reset for each top-level solve call)
    size_t current_depth_;     ///< Current recursion depth
    size_t max_reached_depth_; ///< Maximum depth reached in this solve call
    size_t subgames_created_;  ///< Number of subgames created in this solve call

    // Pre-allocated working sets to avoid repeated allocations
    std::set<graphs::ParityVertex> working_set_;
    std::map<graphs::ParityVertex, graphs::ParityVertex> vertex_mapping_;

    /**
     * @brief Internal recursive solve with depth tracking
     * @param graph Parity graph to solve
     * @param depth Current recursion depth
     * @return Solution with winning regions and strategies
     */
    RecursiveParitySolution solve_internal(const graphs::ParityGraph &graph, size_t depth);

    /**
     * @brief Initialize solver state for a new solve call
     */
    void reset_solve_state();
    /**
     * @brief Create subgame by removing vertices
     * @param graph Original graph
     * @param toRemove Vertices to remove
     * @return Subgame without removed vertices
     */
    graphs::ParityGraph create_subgame(const graphs::ParityGraph &graph,
                                       const std::set<graphs::ParityVertex> &to_remove);

    /**
     * @brief Merge solutions from subgame back to original game
     * @param originalSolution Solution for original game vertices
     * @param subgameSolution Solution for subgame
     * @param vertexMapping Mapping from subgame to original vertices
     * @param graph Original graph for checking vertex ownership
     */
    void merge_solutions(RecursiveParitySolution &original_solution,
                         const RecursiveParitySolution &subgame_solution,
                         const std::map<graphs::ParityVertex, graphs::ParityVertex> &vertex_mapping,
                         const graphs::ParityGraph &graph);
};

} // namespace solvers
} // namespace ggg
