#pragma once

#include "libggg/graphs/mean_payoff_graph.hpp"
#include "libggg/solvers/solver.hpp"
#include <boost/dynamic_bitset.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <map>
#include <vector>

namespace ggg {
namespace solvers {

/**
 * @brief MSCA (Mean-payoff Solver with Constraint Analysis) solver for mean payoff vertex games
 *
 * This solver implements the Dorfman/Kaplan/Zwick algorithm for solving mean payoff vertex games.
 * The algorithm uses a scaling-based approach combined with energy function manipulation to solve
 * mean payoff games efficiently, achieving strongly polynomial time complexity.
 *
 * Based on:
 * - "Simple Stochastic Games with Few Random Vertices are Easy to Solve" (ICALP 2019)
 * - Rectified implementation from arXiv:2310.04130
 *
 * Provides complete solutions with winning regions, strategies, and quantitative values.
 */
using MSCASolutionType = RSQSolution<graphs::MeanPayoffGraph, DeterministicStrategy<graphs::MeanPayoffGraph>, long long>;

class MSCASolver : public Solver<graphs::MeanPayoffGraph, MSCASolutionType> {
  public:
    /**
     * @brief Solve the mean payoff game using MSCA algorithm
     * @param graph Mean payoff graph to solve
     * @return Complete solution with winning regions, strategies, and quantitative values
     * @complexity Time: O(min(mnW,m*2^{n/2})), Space: O(V) where n = vertices, m = edges, W = largest positive edge weight
     */
    RSQSolution<graphs::MeanPayoffGraph, DeterministicStrategy<graphs::MeanPayoffGraph>, long long> solve(const graphs::MeanPayoffGraph &graph) override;

    /**
     * @brief Get solver name
     * @return Solver description
     */
    std::string get_name() const override {
        return "MSCA (Mean-payoff Solver with Constraint Analysis) Solver";
    }

  private:
    using Vertex = graphs::MeanPayoffGraph::vertex_descriptor;
    using Bitset = boost::dynamic_bitset<unsigned long long>;

    // Algorithm state
    long long scaling_val_;
    long long delta_value_;
    long long nw_; // Maximum absolute weight
    int working_vertex_index_;

    // Vertex mappings
    std::map<Vertex, int> vertex_to_index_;
    std::vector<Vertex> index_to_vertex_;

    // Algorithm data structures
    std::vector<long long> weight_;
    std::vector<long long> msrfun_; // Mean-payoff response function
    std::vector<int> count_;
    std::vector<Vertex> strategy_;
    Bitset rescaled_;
    Bitset setL_;
    Bitset setB_;

    // Statistics
    unsigned long count_update_;
    unsigned long count_delta_;
    unsigned long count_iter_delta_;
    unsigned long count_super_delta_;
    unsigned long count_null_delta_;
    unsigned long count_scaling_;
    unsigned long max_delta_;

    const graphs::MeanPayoffGraph *graph_;

    // Core algorithm methods
    void init(const graphs::MeanPayoffGraph &graph);
    long long calc_n_w();
    long long wf(int predecessor_idx, int successor_idx);
    long long delta_p1();
    long long delta_p2();
    void delta();
    void update_func(int pos);
    void update_energy();
    void compute_energy();

    // Utility methods
    bool is_empty() const;
    void reset();
};

} // namespace solvers
} // namespace ggg
