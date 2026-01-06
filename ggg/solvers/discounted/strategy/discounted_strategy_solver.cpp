#include "discounted_strategy_solver.hpp"
#include "libggg/utils/logging.hpp"
#include <boost/graph/graph_utility.hpp>
#include <map>
#include <random>

namespace ggg::solvers {

void DiscountedStrategySolver::switch_str(const graphs::DiscountedGraph &graph) {
    const auto [vertices_begin, vertices_end] = boost::vertices(graph);
    for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
        if (graph[vertex].player == 0) {
            const auto [out_edges_begin, out_edges_end] = boost::out_edges(vertex, graph);
            for (const auto &gedge : boost::make_iterator_range(out_edges_begin, out_edges_end)) {
                const auto SUCCESSOR = boost::target(gedge, graph);
                auto olde = edge(vertex, strategy[vertex], graph);
                auto newe = edge(vertex, SUCCESSOR, graph);
                if (graph[olde.first].weight + graph[olde.first].discount * sol[strategy[vertex]] < graph[newe.first].weight + graph[newe.first].discount * sol[SUCCESSOR]) {
                    strategy[vertex] = SUCCESSOR;
                    switches++;
                }
            }
        }
    }
}

int DiscountedStrategySolver::count_player_edges(const graphs::DiscountedGraph &graph) {
    int edges = 0;
    const auto [vertices_begin, vertices_end] = boost::vertices(graph);
    for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
        if (graph[vertex].player == 0) {
            edges++;
        } else {
            const auto [out_edges_begin, out_edges_end] = boost::out_edges(vertex, graph);
            for (const auto &edge : boost::make_iterator_range(out_edges_begin, out_edges_end)) {
                edges++;
            }
        }
    }
    return edges;
}

void DiscountedStrategySolver::calculate_obj_coefficients(const graphs::DiscountedGraph &graph,
                                                          std::vector<double> &obj_coeff,
                                                          std::vector<double> &var_up,
                                                          std::vector<double> &var_low) {
    const auto [vertices_begin, vertices_end] = boost::vertices(graph);
    obj_coeff.resize(boost::num_vertices(graph));
    var_up.resize(boost::num_vertices(graph));
    var_low.resize(boost::num_vertices(graph));

    for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
        obj_coeff[vertex] = -1;
        var_up[vertex] = std::numeric_limits<double>::infinity();
        var_low[vertex] = -std::numeric_limits<double>::infinity();
    }
}

int DiscountedStrategySolver::setup_matrix_rows(const graphs::DiscountedGraph &graph,
                                                std::vector<std::vector<double>> &matrix_coeff,
                                                std::vector<double> &obj_coeff_up,
                                                std::vector<double> &obj_coeff_low) {
    const auto [vertices_begin, vertices_end] = boost::vertices(graph);
    int row = 0;

    for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
        if (graph[vertex].player == 0) {
            auto curre = edge(vertex, strategy[vertex], graph);
            obj_coeff_up[row] = graph[curre.first].weight;
            obj_coeff_low[row] = graph[curre.first].weight;
            for (const auto &j : boost::make_iterator_range(vertices_begin, vertices_end)) {
                if (j == vertex) {
                    if (j == strategy[vertex]) {
                        matrix_coeff[row][j] = 1.0 - graph[curre.first].discount;
                    } else {
                        matrix_coeff[row][j] = 1.0;
                    }
                } else if (j == strategy[vertex]) {
                    matrix_coeff[row][j] = -1.0 * graph[curre.first].discount;
                } else {
                    matrix_coeff[row][j] = 0.0;
                }
            }
            row++;
        } else {
            const auto [out_edges_begin, out_edges_end] = boost::out_edges(vertex, graph);
            for (const auto &gedge : boost::make_iterator_range(out_edges_begin, out_edges_end)) {
                const auto SUCCESSOR = boost::target(gedge, graph);
                auto curre = edge(vertex, SUCCESSOR, graph);
                obj_coeff_up[row] = graph[curre.first].weight;
                obj_coeff_low[row] = -std::numeric_limits<double>::infinity();
                for (const auto &j : boost::make_iterator_range(vertices_begin, vertices_end)) {
                    if (j == vertex) {
                        if (j == SUCCESSOR) {
                            matrix_coeff[row][j] = 1.0 - graph[curre.first].discount;
                        } else {
                            matrix_coeff[row][j] = 1.0;
                        }
                    } else if (j == SUCCESSOR) {
                        matrix_coeff[row][j] = -1.0 * graph[curre.first].discount;
                    } else {
                        matrix_coeff[row][j] = 0.0;
                    }
                }
                row++;
            }
        }
    }
    return row;
}

void DiscountedStrategySolver::solve_simplex(const std::vector<std::vector<double>> &matrix_coeff,
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

auto DiscountedStrategySolver::solve(const graphs::DiscountedGraph &graph) -> RSQSolution<graphs::DiscountedGraph> {
    LGG_INFO("Starting Strategy Improvement solver for discounted game");

    RSQSolution<graphs::DiscountedGraph> solution(false); // init declare solution
    // Check if graph is valid
    if (!graphs::is_valid(graph)) {
        LGG_ERROR("Invalid discounted graph provided");
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
    strategy.clear();
    sol.clear();

    const auto [vertices_begin, vertices_end] = boost::vertices(graph);

    // Initialize strategies
    for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
        if (graph[vertex].player == 0) {
            const auto [out_begin, out_end] = boost::out_edges(vertex, graph);
            if (out_begin != out_end) { // Warning, error case no else
                strategy[vertex] = boost::target(*out_begin, graph);
            }
        } else {
            strategy[vertex] = 0; // Sentinel value for player 1, can be already set to -1
        }
        sol[vertex] = 0.0; // Initialize solution values
    }

    // Set up matrix dimensions
    int edges = count_player_edges(graph);
    int num_vertices = boost::num_vertices(graph);

    // Initialize matrix and coefficient vectors
    std::vector<std::vector<double>> matrix_coeff(edges, std::vector<double>(num_vertices));
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
    std::vector<double> n_obj_coeff(num_vertices);
    for (std::size_t i = 0; i < n_obj_coeff.size(); ++i) {
        n_obj_coeff[i] = obj_coeff[i] * (-1);
    }

    // Convert sol map to vector for simplex solver
    std::vector<double> sol_vec(num_vertices);
    for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
        sol_vec[vertex] = sol[vertex];
    }

    // Find first solution
    double obj = 0;
    solve_simplex(matrix_coeff, obj_coeff_low, obj_coeff_up, var_low, var_up, n_obj_coeff, sol_vec, obj);

    // Update sol map from vector
    for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
        sol[vertex] = sol_vec[vertex];
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
        for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
            sol[vertex] = sol_vec[vertex];
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