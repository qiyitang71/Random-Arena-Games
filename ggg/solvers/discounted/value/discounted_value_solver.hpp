#pragma once

#include "libggg/graphs/discounted_graph.hpp"
#include "libggg/solvers/solver.hpp"
#include <boost/dynamic_bitset.hpp>
#include <map>
#include "../../uintqueue.hpp"

namespace ggg::solvers {

/**
 * @brief Value Iteration solver for discounted games
 *
 * Value Iteration implementation for discounted games
 * @complexity Time: O(?), Space: O(?) - exponential, bounded by the maximum number of updates
 */
class DiscountedValueSolver : public Solver<graphs::DiscountedGraph, RSQSolution<graphs::DiscountedGraph>> {
  public:
    /**
     * @brief Solve the discounted game using Value Iteration algorithm
     * @param graph Discounted graph to solve
     * @return Solution with winning regions, strategies, and quantitative values
     */
    auto solve(const graphs::DiscountedGraph &graph) -> RSQSolution<graphs::DiscountedGraph> override;

    /**
     * @brief Get solver name
     * @return Solver description
     */
    [[nodiscard]] auto get_name() const -> std::string override {
        return "Value Iteration Discounted Game Solver";
    }

  private:
    // Statistic fields
    uint lifts;      // Total number of lifts needed to reach the fix point
    uint iterations; // Total number of iteration for the game solution
    // Game fields
    const graphs::DiscountedGraph *graph_;
    Uintqueue TAtr;               // Tail queue for positions waiting for attraction
    boost::dynamic_bitset<> BAtr; // Bitset for positions waiting for attraction
    double oldcost;               // Oldcost value of the weights of each node
    std::map<graphs::DiscountedGraph::vertex_descriptor, int> strategy;
    std::map<graphs::DiscountedGraph::vertex_descriptor, double> sol; // Acts as cost function
};

} // namespace ggg::solvers
