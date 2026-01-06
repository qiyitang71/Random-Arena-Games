#pragma once

#include "libggg/graphs/stochastic_discounted_graph.hpp"
#include "libggg/solvers/solver.hpp"
#include <boost/dynamic_bitset.hpp>
#include <map>
#include "../../uintqueue.hpp"

namespace ggg::solvers {

/**
 * @brief Value Iteration solver for stochastic discounted games
 *
 * Value Iteration implementation for stochastic discounted games
 * @complexity Time: O(?), Space: O(?) - exponential, bounded by the maximum number of updates
 */
class StochasticDiscountedValueSolver : public Solver<graphs::Stochastic_DiscountedGraph, RSQSolution<graphs::Stochastic_DiscountedGraph>> {
  public:
    /**
     * @brief Solve the stochastic discounted game using Value Iteration algorithm
     * @param graph Discounted graph to solve
     * @return Solution with winning regions, strategies, and quantitative values
     */
    auto solve(const graphs::Stochastic_DiscountedGraph &graph) -> RSQSolution<graphs::Stochastic_DiscountedGraph> override;

    /**
     * @brief Get solver name
     * @return Solver description
     */
    [[nodiscard]] auto get_name() const -> std::string override {
        return "Value Iteration Stochastic Discounted Game Solver";
    }

  private:
    // Statistic fields
    uint lifts;      // Total number of lifts needed to reach the fix point
    uint iterations; // Total number of iteration for the game solution
    // Game fields
    const graphs::Stochastic_DiscountedGraph *graph_;
    Uintqueue TAtr;               // Tail queue for positions waiting for attraction
    boost::dynamic_bitset<> BAtr; // Bitset for positions waiting for attraction
    double oldcost;               // Oldcost value of the weights of each node
    std::map<graphs::Stochastic_DiscountedGraph::vertex_descriptor, int> strategy;
    std::map<graphs::Stochastic_DiscountedGraph::vertex_descriptor, double> sol; // Acts as cost function
};

} // namespace ggg::solvers
