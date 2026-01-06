#include "stochastic_discounted_objective_solver.hpp"
#include "libggg/utils/logging.hpp"
#include <boost/graph/graph_utility.hpp>
#include <random>

namespace ggg {
namespace solvers {

bool StochasticDiscountedObjectiveSolver::switch_str(const graphs::Stochastic_DiscountedGraph &graph) {
    bool no_switch = true;
    for (const auto &vertex : graphs::get_non_probabilistic_vertices(graph)) {
        const auto OLDE = edge(vertex, strategy[vertex], graph);
        double oldval = graph[OLDE.first].weight;
        const auto REACH = graphs::get_reachable_through_probabilistic(graph, vertex, strategy[vertex]);
        for (const auto &[TARGET, PROB] : REACH) {
            oldval += PROB * graph[OLDE.first].discount * sol[TARGET];
        }
        const auto [out_edges_begin, out_edges_end] = boost::out_edges(vertex, graph);
        for (const auto &gedge : boost::make_iterator_range(out_edges_begin, out_edges_end)) {
            const auto SUCCESSOR = boost::target(gedge, graph);
            if (SUCCESSOR != strategy[vertex]) {
                const auto NEWE = edge(vertex, SUCCESSOR, graph);
                double newval = graph[NEWE.first].weight;
                const auto REACH = graphs::get_reachable_through_probabilistic(graph, vertex, SUCCESSOR);
                for (const auto &[TARGET, PROB] : REACH) {
                    newval += PROB * graph[NEWE.first].discount * sol[TARGET];
                }
                if (((graph[vertex].player == 0) && (oldval + 1e-6 < newval)) || ((graph[vertex].player == 1) && (oldval > 1e-6 + newval))) { // Precision stop
                    strategy[vertex] = SUCCESSOR;
                    switches++;
                    no_switch = false;
                }
            }
        }
    }
    return no_switch;
}

int StochasticDiscountedObjectiveSolver::setup_matrix_rows(const graphs::Stochastic_DiscountedGraph &graph,
                                                           std::vector<std::vector<double>> &matrix_coeff,
                                                           std::vector<double> &obj_coeff_up,
                                                           std::vector<double> &obj_coeff_low,
                                                           std::vector<double> &var_up,
                                                           std::vector<double> &var_low) {
    int row = 0;
    var_up.resize(num_real_vertices);
    var_low.resize(num_real_vertices);

    for (const auto &vertex : graphs::get_non_probabilistic_vertices(graph)) {
        std::fill(matrix_coeff[row].begin(), matrix_coeff[row].end(), 0.0); // Cleanup matrix_coeff[row][vertex] = 0
        var_up[matrixMap[vertex]] = std::numeric_limits<double>::infinity();
        var_low[matrixMap[vertex]] = -std::numeric_limits<double>::infinity();
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
            const auto REACH = graphs::get_reachable_through_probabilistic(graph, vertex, SUCCESSOR);
            matrix_coeff[row][matrixMap[vertex]] = 1.0; // Will be overwritten if vertex is a target
            for (const auto &[TARGET, PROB] : REACH) {
                if (TARGET == vertex) {
                    matrix_coeff[row][matrixMap[TARGET]] = 1.0 - PROB * graph[CURRE.first].discount;
                } else {
                    matrix_coeff[row][matrixMap[TARGET]] = -PROB * graph[CURRE.first].discount;
                }
            }
            row++;
        }
    }
    return row;
}

void StochasticDiscountedObjectiveSolver::calculate_obj_coefficients(const graphs::Stochastic_DiscountedGraph &graph,
                                                                     std::vector<double> &obj_coeff) {
    cff = 0;
    obj_coeff.resize(num_real_vertices);
    for (const auto &vertex : graphs::get_non_probabilistic_vertices(graph)) {
        const auto CURRE = edge(vertex, strategy[vertex], graph);
        const auto REACH = graphs::get_reachable_through_probabilistic(graph, vertex, strategy[vertex]);
        if (graph[vertex].player == 0) {
            obj_coeff[matrixMap[vertex]] += 1;
            for (const auto &[TARGET, PROB] : REACH) {
                obj_coeff[matrixMap[TARGET]] += -PROB * graph[CURRE.first].discount;
            }
            cff += -(graph[CURRE.first].weight);
        } else {
            obj_coeff[matrixMap[vertex]] += -1;
            for (const auto &[TARGET, PROB] : REACH) {
                obj_coeff[matrixMap[TARGET]] += PROB * graph[CURRE.first].discount;
            }
            cff += graph[CURRE.first].weight;
        }
    }
}

void StochasticDiscountedObjectiveSolver::solve_simplex(Simplex &solver,
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

auto StochasticDiscountedObjectiveSolver::solve(const graphs::Stochastic_DiscountedGraph &graph) -> RSQSolution<graphs::Stochastic_DiscountedGraph> {
    LGG_INFO("Starting objective improvement solver for stochastic discounted game");

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
    stales = 0;

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
    for (const auto &vertex : graphs::get_non_probabilistic_vertices(graph)) {
        const auto [out_edges_begin, out_edges_end] = boost::out_edges(vertex, graph);
        for (const auto &edge : boost::make_iterator_range(out_edges_begin, out_edges_end)) {
            edges++;
        }
    }
    num_real_vertices = boost::distance(graphs::get_non_probabilistic_vertices(graph));

    // Initialize matrix and coefficient vectors
    std::vector<std::vector<double>> matrix_coeff(edges, std::vector<double>(num_real_vertices));
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
    std::vector<double> n_obj_coeff(num_real_vertices);
    for (std::size_t i = 0; i < n_obj_coeff.size(); ++i) {
        n_obj_coeff[i] = obj_coeff[i] * (-1);
    }

    // Find first solution
    double obj = 0;
    std::vector<double> sol_vec(num_real_vertices);
    Simplex solver(matrix_coeff, obj_coeff_low, obj_coeff_up, var_low, var_up, n_obj_coeff);
    solve_simplex(solver, matrix_coeff, obj_coeff_low, obj_coeff_up, var_low, var_up, n_obj_coeff, sol_vec, obj);
    solver.purge_artificial_columns();

    // Update sol map from vector
    for (size_t i = 0; i < sol_vec.size(); ++i) {
        sol[reverseMap[i]] = sol_vec[i];
    }
    bool stale = false;
    std::map<graphs::Stochastic_DiscountedGraph::vertex_descriptor, std::list<int>> stale_str;
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
                    for (const auto &vertex : graphs::get_non_probabilistic_vertices(graph)) {
                        const auto OLDE = edge(vertex, strategy[vertex], graph);
                        double oldval = graph[OLDE.first].weight;
                        const auto REACH = graphs::get_reachable_through_probabilistic(graph, vertex, strategy[vertex]);
                        for (const auto &[TARGET, PROB] : REACH) {
                            oldval += PROB * graph[OLDE.first].discount * sol[TARGET];
                        }
                        const auto [out_edges_begin, out_edges_end] = boost::out_edges(vertex, graph);
                        for (const auto &gedge : boost::make_iterator_range(out_edges_begin, out_edges_end)) {
                            const auto SUCCESSOR = boost::target(gedge, graph);
                            if (SUCCESSOR != strategy[vertex]) {
                                const auto NEWE = edge(vertex, SUCCESSOR, graph);
                                double newval = graph[NEWE.first].weight;
                                const auto REACH = graphs::get_reachable_through_probabilistic(graph, vertex, SUCCESSOR);
                                for (const auto &[TARGET, PROB] : REACH) {
                                    newval += PROB * graph[NEWE.first].discount * sol[TARGET];
                                }
                                stalevalue = oldval - newval;
                                if (std::abs(stalevalue) < 1e-8) {
                                    stale_str[vertex].push_back(SUCCESSOR);
                                    nr_str++;
                                }
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

} // namespace solvers
} // namespace ggg