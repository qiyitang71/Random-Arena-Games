#include "stochastic_discounted_strategy_solver.hpp"
#include "libggg/utils/logging.hpp"
#include <boost/graph/graph_utility.hpp>
#include <map>
#include <random>

namespace ggg::solvers {

void StochasticDiscountedStrategySolver::switch_str(const graphs::Stochastic_DiscountedGraph &graph) {
    const auto [vertices_begin, vertices_end] = boost::vertices(graph);
    for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
        if (graph[vertex].player == 0) {
            const auto OLDE = edge(vertex, strategy[vertex], graph);
            double oldval = graph[OLDE.first].weight;
            const auto REACH = graphs::get_reachable_through_probabilistic(graph, vertex, strategy[vertex]);
            for (const auto &[TARGET, PROB] : REACH) {
                oldval += PROB * graph[OLDE.first].discount * sol[TARGET];
            }
            const auto [out_edges_begin, out_edges_end] = boost::out_edges(vertex, graph);
            for (const auto &gedge : boost::make_iterator_range(out_edges_begin, out_edges_end)) {
                const auto SUCCESSOR = boost::target(gedge, graph);
                const auto NEWE = edge(vertex, SUCCESSOR, graph);
                double newval = graph[NEWE.first].weight;
                const auto REACH = graphs::get_reachable_through_probabilistic(graph, vertex, SUCCESSOR);
                for (const auto &[TARGET, PROB] : REACH) {
                    newval += PROB * graph[NEWE.first].discount * sol[TARGET];
                }
                if (oldval + 1e-6 < newval) { // Precision stop
                    strategy[vertex] = SUCCESSOR;
                    switches++;
                }
            }
        }
    }
}

int StochasticDiscountedStrategySolver::count_player_edges(const graphs::Stochastic_DiscountedGraph &graph) {
    int edges = 0;
    const auto [vertices_begin, vertices_end] = boost::vertices(graph);
    for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
        if (graph[vertex].player == 0) {
            edges++;
        } else {
            if (graph[vertex].player == 1) {
                const auto [out_edges_begin, out_edges_end] = boost::out_edges(vertex, graph);
                for (const auto &edge : boost::make_iterator_range(out_edges_begin, out_edges_end)) {
                    edges++;
                }
            }
        }
    }
    return edges;
}

void StochasticDiscountedStrategySolver::calculate_obj_coefficients(const graphs::Stochastic_DiscountedGraph &graph,
                                                                    std::vector<double> &obj_coeff,
                                                                    std::vector<double> &var_up,
                                                                    std::vector<double> &var_low) {
    obj_coeff.resize(num_real_vertices);
    var_up.resize(num_real_vertices);
    var_low.resize(num_real_vertices);

    for (const auto &vertex : graphs::get_non_probabilistic_vertices(graph)) {
        obj_coeff[matrixMap[vertex]] = -1;
        var_up[matrixMap[vertex]] = std::numeric_limits<double>::infinity();
        var_low[matrixMap[vertex]] = -std::numeric_limits<double>::infinity();
    }
}

int StochasticDiscountedStrategySolver::setup_matrix_rows(const graphs::Stochastic_DiscountedGraph &graph,
                                                          std::vector<std::vector<double>> &matrix_coeff,
                                                          std::vector<double> &obj_coeff_up,
                                                          std::vector<double> &obj_coeff_low) {
    int row = 0;
    for (const auto &vertex : graphs::get_non_probabilistic_vertices(graph)) {
        std::fill(matrix_coeff[row].begin(), matrix_coeff[row].end(), 0.0); // Cleanup matrix_coeff[row][vertex] = 0
        if (graph[vertex].player == 0) {
            const auto CURRE = edge(vertex, strategy[vertex], graph);
            obj_coeff_up[row] = graph[CURRE.first].weight;
            obj_coeff_low[row] = graph[CURRE.first].weight;
            const auto REACH = graphs::get_reachable_through_probabilistic(graph, vertex, strategy[vertex]);
            matrix_coeff[row][matrixMap[vertex]] = 1.0; // Will be overwritten if vertex is a target
            for (const auto &[TARGET, PROB] : REACH) {
                if (TARGET == vertex) {
                    matrix_coeff[row][matrixMap[TARGET]] = 1.0 - PROB * graph[CURRE.first].discount;
                } else {
                    matrix_coeff[row][matrixMap[TARGET]] = -1.0 * PROB * graph[CURRE.first].discount;
                }
            }
            row++;
        } else {
            const auto [out_edges_begin, out_edges_end] = boost::out_edges(vertex, graph);
            for (const auto &gedge : boost::make_iterator_range(out_edges_begin, out_edges_end)) {
                const auto SUCCESSOR = boost::target(gedge, graph);
                const auto CURRE = edge(vertex, SUCCESSOR, graph);
                obj_coeff_up[row] = graph[CURRE.first].weight;
                obj_coeff_low[row] = -std::numeric_limits<double>::infinity();
                const auto REACH = graphs::get_reachable_through_probabilistic(graph, vertex, SUCCESSOR);
                matrix_coeff[row][matrixMap[vertex]] = 1.0; // Will be overwritten if vertex is a target
                for (const auto &[TARGET, PROB] : REACH) {
                    if (TARGET == vertex) {
                        matrix_coeff[row][matrixMap[TARGET]] = 1.0 - PROB * graph[CURRE.first].discount;
                    } else {
                        matrix_coeff[row][matrixMap[TARGET]] = -1.0 * PROB * graph[CURRE.first].discount;
                    }
                }
                row++;
            }
        }
    }
    return row;
}

void StochasticDiscountedStrategySolver::solve_simplex(const std::vector<std::vector<double>> &matrix_coeff,
                                                       const std::vector<double> &obj_coeff_low,
                                                       const std::vector<double> &obj_coeff_up,
                                                       const std::vector<double> &var_low,
                                                       const std::vector<double> &var_up,
                                                       const std::vector<double> &n_obj_coeff,
                                                       std::vector<double> &sol_vec,
                                                       double &obj) {
    Simplex solver(matrix_coeff, obj_coeff_low, obj_coeff_up, var_low, var_up, n_obj_coeff);
    while (solver.remove_artificial_variables()) {
        // solver.printTableau();
    }
    while (solver.calculate_simplex()) {
        lpiter++;
    }
    solver.get_full_results(sol_vec, obj, true);
}

auto StochasticDiscountedStrategySolver::solve(const graphs::Stochastic_DiscountedGraph &graph) -> RSQSolution<graphs::Stochastic_DiscountedGraph> {
    LGG_INFO("Starting Strategy Improvement solver for stochastic discounted game");

    RSQSolution<graphs::Stochastic_DiscountedGraph> solution(false); // init declare solution
    // Check if graph is valid
    if (!graphs::is_valid(graph)) {
        LGG_ERROR("Invalid stochastic discounted graph provided");
        solution.set_valid(false);
        return solution;
    }
    if (boost::num_vertices(graph) == 0) {
        LGG_WARN("Empty graph provided");
        return solution;
    }

    // Initialize solver state
    switches = 0;
    iterations = 0;
    lpiter = 0;

    // Initialize strategy map and solution map
    const auto [vertices_begin, vertices_end] = boost::vertices(graph);
    matrixMap.clear();
    int reindex = 0;
    for (const auto &vertex : graphs::get_non_probabilistic_vertices(graph)) {
        matrixMap[vertex] = reindex;
        reverseMap[reindex] = vertex;
        ++reindex;
    }
    for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
        if (matrixMap.count(vertex) == 0) {
            matrixMap[vertex] = reindex;
            reverseMap[reindex] = vertex;
            ++reindex;
        }
    }
    strategy.clear();
    sol.clear();

    // Initialize strategies
    for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
        if (graph[vertex].player == 0) {
            const auto [out_begin, out_end] = boost::out_edges(vertex, graph);
            if (out_begin != out_end) { // Warning, error case no else
                strategy[vertex] = boost::target(*out_begin, graph);
            }
        } else {
            strategy[vertex] = 0; // Sentinel value for player 1 and -1, can be already set to -1
        }
        sol[vertex] = 0.0; // Initialize solution values
    }

    // Set up matrix dimensions
    int edges = count_player_edges(graph);
    int num_vertices = boost::num_vertices(graph);
    num_real_vertices = boost::distance(graphs::get_non_probabilistic_vertices(graph));

    // Initialize matrix and coefficient vectors
    std::vector<std::vector<double>> matrix_coeff(edges, std::vector<double>(num_real_vertices));
    std::vector<double> obj_coeff_up(edges);
    std::vector<double> obj_coeff_low(edges);
    std::vector<double> obj_coeff;
    std::vector<double> var_up;
    std::vector<double> var_low;

    // Calculate objective coefficients
    calculate_obj_coefficients(graph, obj_coeff, var_up, var_low);

    // Set up initial matrix rows
    setup_matrix_rows(graph, matrix_coeff, obj_coeff_up, obj_coeff_low);

    // Prepare negated objective coefficients for maximization
    std::vector<double> n_obj_coeff(num_real_vertices);
    for (std::size_t i = 0; i < n_obj_coeff.size(); ++i) {
        n_obj_coeff[i] = obj_coeff[i] * (-1);
    }

    // Find first solution
    double obj = 0;
    std::vector<double> sol_vec(num_real_vertices);
    solve_simplex(matrix_coeff, obj_coeff_low, obj_coeff_up, var_low, var_up, n_obj_coeff, sol_vec, obj);

    // Update sol map from vector
    for (size_t i = 0; i < sol_vec.size(); ++i) {
        sol[reverseMap[i]] = sol_vec[i];
    }

    double old_obj = obj - 1; // trigger iteration

    // Main improvement loop
    while (old_obj < obj) {
        iterations++;
        old_obj = obj;
        switch_str(graph);

        // Update matrix rows with new strategy
        setup_matrix_rows(graph, matrix_coeff, obj_coeff_up, obj_coeff_low);

        // Solve with updated matrix
        solve_simplex(matrix_coeff, obj_coeff_low, obj_coeff_up, var_low, var_up, n_obj_coeff, sol_vec, obj);

        // Update sol map from vector
        for (size_t i = 0; i < sol_vec.size(); ++i) {
            sol[reverseMap[i]] = sol_vec[i];
        }
    }

    // Set solution results
    for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
        if (sol[vertex] >= 0) {
            solution.set_winning_player(vertex, 0);
        } else {
            solution.set_winning_player(vertex, 1);
        }
        if (graph[vertex].player == 0) {
            solution.set_strategy(vertex, strategy[vertex]);
        } else {
            solution.set_strategy(vertex, -1); // Sentinel value for player 1
        }
        solution.set_value(vertex, sol[vertex]);
    }

    LGG_TRACE("Solved with ", iterations, " iterations");
    LGG_TRACE("Solved with ", lpiter, " LP pivotes");
    LGG_TRACE("Solved with ", switches, " switches");
    solution.set_solved(true);
    return solution;
}

} // namespace ggg::solvers
