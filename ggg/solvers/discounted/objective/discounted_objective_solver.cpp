#include "discounted_objective_solver.hpp"
#include "libggg/utils/logging.hpp"
#include <boost/graph/graph_utility.hpp>
#include <random>

namespace ggg::solvers {

bool DiscountedObjectiveSolver::switch_str(const graphs::DiscountedGraph &graph) {
    bool no_switch = true;
    const auto [vertices_begin, vertices_end] = boost::vertices(graph);
    for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
        const auto [out_edges_begin, out_edges_end] = boost::out_edges(vertex, graph);
        const auto OLDE = edge(vertex, strategy[vertex], graph);
        const double OLDEDGE = graph[OLDE.first].weight + graph[OLDE.first].discount * sol[strategy[vertex]];
        for (const auto &gedge : boost::make_iterator_range(out_edges_begin, out_edges_end)) {
            const auto SUCCESSOR = boost::target(gedge, graph);
            if (SUCCESSOR != strategy[vertex]) {
                const auto NEWE = edge(vertex, SUCCESSOR, graph);
                const double NEWEDGE = graph[NEWE.first].weight + graph[NEWE.first].discount * sol[SUCCESSOR];
                if (graph[vertex].player == 0) {
                    if (OLDEDGE + 1e-8 < NEWEDGE) {
                        strategy[vertex] = SUCCESSOR;
                        switches++;
                        no_switch = false;
                    }
                } else {
                    if (OLDEDGE > NEWEDGE + 1e-8) {
                        strategy[vertex] = SUCCESSOR;
                        switches++;
                        no_switch = false;
                    }
                }
            }
        }
    }
    return no_switch;
}

int DiscountedObjectiveSolver::setup_matrix_rows(const graphs::DiscountedGraph &graph,
                                                 std::vector<std::vector<double>> &matrix_coeff,
                                                 std::vector<double> &obj_coeff_up,
                                                 std::vector<double> &obj_coeff_low,
                                                 std::vector<double> &var_up,
                                                 std::vector<double> &var_low) {
    const auto [vertices_begin, vertices_end] = boost::vertices(graph);
    int row = 0;
    var_up.resize(boost::num_vertices(graph));
    var_low.resize(boost::num_vertices(graph));

    for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
        var_up[vertex] = std::numeric_limits<double>::infinity();
        var_low[vertex] = -std::numeric_limits<double>::infinity();
        const auto [out_edges_begin, out_edges_end] = boost::out_edges(vertex, graph);
        for (const auto &gedge : boost::make_iterator_range(out_edges_begin, out_edges_end)) {
            const auto SUCCESSOR = boost::target(gedge, graph);
            const auto CURRE = edge(vertex, SUCCESSOR, graph);
            if (graph[vertex].player == 0) {
                obj_coeff_up[row] = std::numeric_limits<double>::infinity();
                obj_coeff_low[row] = graph[CURRE.first].weight;
            } else {
                obj_coeff_up[row] = graph[CURRE.first].weight;
                obj_coeff_low[row] = -std::numeric_limits<double>::infinity();
            }
            for (const auto &j : boost::make_iterator_range(vertices_begin, vertices_end)) {
                if (j == vertex) {
                    if (j == SUCCESSOR) {
                        matrix_coeff[row][j] = 1.0 - graph[CURRE.first].discount;
                    } else {
                        matrix_coeff[row][j] = 1.0;
                    }
                } else if (j == SUCCESSOR) {
                    matrix_coeff[row][j] = -1.0 * graph[CURRE.first].discount;
                } else {
                    matrix_coeff[row][j] = 0.0;
                }
            }
            row++;
        }
    }
    return row;
}

void DiscountedObjectiveSolver::calculate_obj_coefficients(const graphs::DiscountedGraph &graph,
                                                           std::vector<double> &obj_coeff) {
    cff = 0;
    const auto [vertices_begin, vertices_end] = boost::vertices(graph);
    obj_coeff.resize(boost::num_vertices(graph));
    for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
        obj_coeff[vertex] = 0;
    }
    for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
        const auto CURRE = edge(vertex, strategy[vertex], graph);
        if (graph[vertex].player == 0) {
            if (vertex == strategy[vertex]) {
                obj_coeff[vertex] += 1 - graph[CURRE.first].discount;
            } else {
                obj_coeff[vertex] += 1;
                obj_coeff[strategy[vertex]] += -(graph[CURRE.first].discount);
            }
            cff += -(graph[CURRE.first].weight);

        } else {
            if (vertex == strategy[vertex]) {
                obj_coeff[vertex] += graph[CURRE.first].discount - 1;
            } else {
                obj_coeff[vertex] += -1;
                obj_coeff[strategy[vertex]] += graph[CURRE.first].discount;
            }
            cff += graph[CURRE.first].weight;
        }
    }
}

void DiscountedObjectiveSolver::solve_simplex(Simplex &solver,
                                              const std::vector<std::vector<double>> &matrix_coeff,
                                              const std::vector<double> &obj_coeff_low,
                                              const std::vector<double> &obj_coeff_up,
                                              const std::vector<double> &var_low,
                                              const std::vector<double> &var_up,
                                              const std::vector<double> &n_obj_coeff,
                                              std::vector<double> &sol_vec,
                                              double &obj) {
    while (solver.remove_artificial_variables()) {
        // solver.printTableau();
    }
    while (solver.calculate_simplex()) {
        lpiter++;
    }
    solver.get_full_results(sol_vec, obj, true);
}

auto DiscountedObjectiveSolver::solve(const graphs::DiscountedGraph &graph) -> RSQSolution<graphs::DiscountedGraph> {
    LGG_INFO("Starting Objective Improvement solver for discounted game");

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
    stales = 0;

    // Initialize strategy map and solution map
    strategy.clear();
    sol.clear();

    const auto [vertices_begin, vertices_end] = boost::vertices(graph);

    // Initialize strategy and solution value
    for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
        const auto [out_begin, out_end] = boost::out_edges(vertex, graph);
        if (out_begin != out_end) { // Warning, error case no else
            strategy[vertex] = boost::target(*out_begin, graph);
        }
        sol[vertex] = 0.0;
    }

    // Set up matrix dimensions
    int edges = 0;
    for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
        const auto [out_edges_begin, out_edges_end] = boost::out_edges(vertex, graph);
        for (const auto &edge : boost::make_iterator_range(out_edges_begin, out_edges_end)) {
            edges++;
        }
    }
    int num_vertices = boost::num_vertices(graph);

    // Initialize matrix and coefficient vectors
    std::vector<std::vector<double>> matrix_coeff(edges, std::vector<double>(num_vertices));
    std::vector<double> obj_coeff_up(edges);
    std::vector<double> obj_coeff_low(edges);
    std::vector<double> obj_coeff;
    std::vector<double> var_up;
    std::vector<double> var_low;

    // Calculate objective coefficients
    calculate_obj_coefficients(graph, obj_coeff);

    // Set up initial matrix rows
    setup_matrix_rows(graph, matrix_coeff, obj_coeff_up, obj_coeff_low, var_up, var_low);

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
    Simplex solver(matrix_coeff, obj_coeff_low, obj_coeff_up, var_low, var_up, n_obj_coeff);
    solve_simplex(solver, matrix_coeff, obj_coeff_low, obj_coeff_up, var_low, var_up, n_obj_coeff, sol_vec, obj);
    solver.purge_artificial_columns();

    // Update sol map from vector
    for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
        sol[vertex] = sol_vec[vertex];
    }
    bool stale = false;
    std::map<graphs::DiscountedGraph::vertex_descriptor, std::list<int>> stale_str;
    bool improving = true;
    int nr_str = 0;
    double stalevalue = 0;
    while (!stale && (cff - obj > 1e-8)) // Note, was +obj, sign changed due to new LP solver
    {
        stale = switch_str(graph);
        // TODO export stale game handler as a function
        if (stale) {
            stales++;
            if (nr_str == 0) {
                if (improving) // Got stuck on a stale game from an improving one, then computes all stale strategies
                {
                    improving = false;
                    for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
                        const auto [out_edges_begin, out_edges_end] = boost::out_edges(vertex, graph);
                        for (const auto &gedge : boost::make_iterator_range(out_edges_begin, out_edges_end)) {
                            const auto SUCCESSOR = boost::target(gedge, graph);
                            const auto CURRE = edge(vertex, SUCCESSOR, graph);
                            stalevalue = sol[vertex] - (graph[CURRE.first].discount * sol[SUCCESSOR]) - graph[CURRE.first].weight;
                            if (SUCCESSOR != strategy[vertex] && (std::abs(stalevalue) < 1e-8)) {
                                stale_str[vertex].push_back(SUCCESSOR);
                                nr_str++;
                            }
                        }
                    }
                } else {
                    break; // Prevents from looping on stale strategies
                }
            }
            // If it has already computed stale strategies, then try one more
            for (const auto &[key, lst] : stale_str) {
                if (!lst.empty()) {
                    strategy[key] = lst.front(); // Stale strategy extraction
                    stale_str[key].pop_front();
                    stale = false;
                    nr_str--;
                    break;
                }
            }
        } else {
            iterations++;
            improving = true;
            stale_str.clear();
        }

        // Calculate objective coefficients
        calculate_obj_coefficients(graph, obj_coeff);

        // Prepare negated objective coefficients for maximization
        for (std::size_t i = 0; i < n_obj_coeff.size(); ++i) {
            n_obj_coeff[i] = obj_coeff[i] * (-1);
        }

        // Find next solution
        solver.update_objective_row(n_obj_coeff, 0);
        solver.normalize_objective_row();
        solve_simplex(solver, matrix_coeff, obj_coeff_low, obj_coeff_up, var_low, var_up, n_obj_coeff, sol_vec, obj);
    }

    if (cff - obj > 1e-8) // Was +obj, sign changed due to LP solver
    {
        LGG_INFO("Warning, stopping with no local improvements, solution not optimal");
    }

    // Set solution results
    for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
        if (sol[vertex] >= 0) {
            solution.set_winning_player(vertex, 0);
        } else {
            solution.set_winning_player(vertex, 1);
        }
        solution.set_strategy(vertex, strategy[vertex]);
        solution.set_value(vertex, sol[vertex]);
    }

    LGG_TRACE("Solved with ", iterations, " iterations");
    LGG_TRACE("Solved with ", lpiter, " LP pivotes");
    LGG_TRACE("Solved with ", switches, " switches");
    LGG_TRACE("Solved with ", stales, " stales");
    solution.set_solved(true);
    return solution;
}

} // namespace ggg::solvers