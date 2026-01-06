#include "stochastic_discounted_value_solver.hpp"
#include "libggg/utils/logging.hpp"
#include <boost/graph/graph_utility.hpp>
#include <map>
#include <random>

namespace ggg::solvers {

auto StochasticDiscountedValueSolver::solve(const graphs::Stochastic_DiscountedGraph &graph) -> RSQSolution<graphs::Stochastic_DiscountedGraph> {
    LGG_INFO("Starting Value Iteration solver for stochastic discounted game");

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
    lifts = 0;
    iterations = 0;

    // Initialize strategy map and solution map
    int num_vertices = boost::num_vertices(graph);
    strategy.clear();
    sol.clear();
    TAtr.resize(num_vertices);
    BAtr.resize(num_vertices);

    const auto [vertices_begin, vertices_end] = boost::vertices(graph);

    // Initialize strategies and add all vertices to queue
    for (const auto &vertex : graphs::get_non_probabilistic_vertices(graph)) {
        strategy[vertex] = -1; // Sentinel value for non set strategy
        sol[vertex] = 0.0;     // Initialize solution values
        // Add all vertices to the queue for processing
        TAtr.push(vertex);
        BAtr[vertex] = true;
    }

    // Main improvement loop
    int pos;
    int best_succ;
    double sum;
    double best;
    while (TAtr.nonempty()) {
        iterations++;
        pos = TAtr.pop();
        BAtr[pos] = false;
        oldcost = sol[pos];
        best_succ = -1;
        const auto [out_edges_begin, out_edges_end] = boost::out_edges(pos, graph);
        for (const auto &gedge : boost::make_iterator_range(out_edges_begin, out_edges_end)) {
            const auto SUCCESSOR = boost::target(gedge, graph);
            const auto CURRE = edge(pos, SUCCESSOR, graph);
            const auto REACH = graphs::get_reachable_through_probabilistic(graph, pos, SUCCESSOR);
            if (best_succ == -1) {
                best_succ = SUCCESSOR;
                best = 0;
                for (const auto &[TARGET, PROB] : REACH) {
                    best += (PROB * graph[CURRE.first].discount * sol[TARGET]);
                }
                best += graph[CURRE.first].weight;
            } else {
                sum = 0;
                for (const auto &[TARGET, PROB] : REACH) {
                    sum += (PROB * graph[CURRE.first].discount * sol[TARGET]);
                }
                sum += graph[CURRE.first].weight;
                // Player 0 maximizes, Player 1 minimizes
                if ((graph[pos].player == 0 && sum > best) || (graph[pos].player == 1 && sum < best)) { // Warning precision comparison
                    best_succ = SUCCESSOR;
                    best = sum;
                }
            }
        }
        if (sol[pos] != best || strategy[pos] == -1) {
            lifts++;
            sol[pos] = best;
            strategy[pos] = best_succ;

            // Propagate change to all predecessors
            for (auto pred : boost::make_iterator_range(boost::vertices(graph))) {
                for (auto e : boost::make_iterator_range(boost::out_edges(pred, graph))) {
                    if (boost::target(e, graph) == pos) {
                        if (!BAtr[pred]) {
                            TAtr.push(pred);
                            BAtr[pred] = true;
                        }
                    }
                }
            }
        }
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
    LGG_TRACE("Solved with ", lifts, " lifts");
    solution.set_solved(true);
    return solution;
}

} // namespace ggg::solvers