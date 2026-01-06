#pragma once

#include "libggg/graphs/discounted_graph.hpp"
#include "libggg/solvers/solver.hpp"
#include <map>
#include "../../Simplex.hpp"

namespace ggg::solvers {

/**
 * @brief Objective Improvement solver for discounted games
 *
 * Objective Improvement implementation for discounted games
 * @complexity Time: O(n^n), Space: O(?) - exponential in the number of strategies
 */
class DiscountedObjectiveSolver : public Solver<graphs::DiscountedGraph, RSQSolution<graphs::DiscountedGraph>> {
  public:
    /**
     * @brief Solve the discounted game using objective improvement algorithm
     * @param graph Discounted graph to solve
     * @return Solution with winning regions, strategies, and quantitative values
     */
    auto solve(const graphs::DiscountedGraph &graph) -> RSQSolution<graphs::DiscountedGraph> override;

    /**
     * @brief Get solver name
     * @return Solver description
     */
    [[nodiscard]] auto get_name() const -> std::string override {
        return "Objective Improvement Discounted Game Solver";
    }

  private:
    /**
     * @brief Updates the current strategy for the discounted game based on the current graph state.
     * @param graph The discounted graph used to update the strategy.
     */
    bool switch_str(const graphs::DiscountedGraph &graph);

    /**
     * @brief Sets up matrix rows for the linear program
     * @param graph The discounted graph
     * @param matrix_coeff Matrix coefficients to fill
     * @param obj_coeff_up Upper bounds for objective coefficients
     * @param obj_coeff_low Lower bounds for objective coefficients
     * @param var_up Upper variable bounds to fill
     * @param var_low Lower variable bounds to fill
     * @return Number of rows created
     */
    int setup_matrix_rows(const graphs::DiscountedGraph &graph,
                          std::vector<std::vector<double>> &matrix_coeff,
                          std::vector<double> &obj_coeff_up,
                          std::vector<double> &obj_coeff_low,
                          std::vector<double> &var_up,
                          std::vector<double> &var_low);

    /**
     * @brief Calculates objective coefficients for the linear program
     * @param graph The discounted graph
     * @param obj_coeff Objective coefficients to fill
     */
    void calculate_obj_coefficients(const graphs::DiscountedGraph &graph,
                                    std::vector<double> &obj_coeff);

    /**
     * @brief Encapsulates simplex solving process
     * @param matrix_coeff Matrix coefficients
     * @param obj_coeff_low Lower bounds for objective coefficients
     * @param obj_coeff_up Upper bounds for objective coefficients
     * @param var_low Lower variable bounds
     * @param var_up Upper variable bounds
     * @param n_obj_coeff Negated objective coefficients
     * @param sol Solution vector to fill
     * @param obj Objective value to fill
     */
    void solve_simplex(Simplex &solver,
                       const std::vector<std::vector<double>> &matrix_coeff,
                       const std::vector<double> &obj_coeff_low,
                       const std::vector<double> &obj_coeff_up,
                       const std::vector<double> &var_low,
                       const std::vector<double> &var_up,
                       const std::vector<double> &n_obj_coeff,
                       std::vector<double> &sol,
                       double &obj);

    // Statistic fields
    uint switches;   // Total number of lifts needed to reach the fix point
    uint iterations; // Total number of iteration for the game solution
    uint lpiter;     // Total number of LP iteration for the game solution
    uint stales;     // Total number of stale iterations
    // Game fields
    const graphs::DiscountedGraph *graph_;
    std::map<graphs::DiscountedGraph::vertex_descriptor, int> strategy; // Strategies of the players
    double cff;                                                         // Objective function coefficient
    std::map<graphs::DiscountedGraph::vertex_descriptor, double> sol;
    std::vector<double> obj_coeff;
};

} // namespace ggg::solvers