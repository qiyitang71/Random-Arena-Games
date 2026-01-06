#include "mse_solver.hpp"
#include "libggg/utils/logging.hpp"
#include <map>
#include <queue>

namespace ggg {
namespace solvers {

RSQSolution<graphs::MeanPayoffGraph, DeterministicStrategy<graphs::MeanPayoffGraph>, int> MSESolver::solve(const graphs::MeanPayoffGraph &graph) {
    LGG_DEBUG("Mean payoff MSE solver starting with ", boost::num_vertices(graph), " vertices");

    // Initialize solution
    RSQSolution<graphs::MeanPayoffGraph, DeterministicStrategy<graphs::MeanPayoffGraph>, int> solution;

    // Base case: empty game
    if (boost::num_vertices(graph) == 0) {
        LGG_TRACE("Empty game - returning solved");
        solution.set_solved(true);
        return solution;
    }

    // Algorithm state variables
    int iterations = 0;
    int lifts = 0;
    int limit = 0;

    std::vector<graphs::MeanPayoffGraph::vertex_descriptor> current_strategy(
        boost::num_vertices(graph),
        boost::graph_traits<graphs::MeanPayoffGraph>::null_vertex());
    std::vector<int> current_cost(boost::num_vertices(graph), 0);
    std::vector<int> current_count(boost::num_vertices(graph), 0);
    std::queue<graphs::MeanPayoffGraph::vertex_descriptor> t_atr;
    std::vector<bool> b_atr(boost::num_vertices(graph), false);

    // Map vertex to appropriate index for above vectors and queue
    std::map<graphs::MeanPayoffGraph::vertex_descriptor, int> vertex_map;
    std::vector<graphs::MeanPayoffGraph::vertex_descriptor> index_to_vertex(boost::num_vertices(graph));

    const auto [vertices_begin, vertices_end] = boost::vertices(graph);
    int vertex_count = 0;

    // Initialize vertex mappings and initial state
    for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
        vertex_map[vertex] = vertex_count;
        index_to_vertex[vertex_count] = vertex;
        vertex_count++;

        const auto WEIGHT = graph[vertex].weight;

        if (WEIGHT > 0) {
            t_atr.push(vertex);
            b_atr[vertex_map[vertex]] = true;
            limit += WEIGHT;
        } else {
            if (graph[vertex].player) {
                current_count[vertex_map[vertex]] = boost::out_degree(vertex, graph);
            }
        }
    }
    limit += 1;

    // Main solution cycle
    while (!t_atr.empty()) {
        iterations++;
        const auto POS = t_atr.front();
        t_atr.pop();
        b_atr[vertex_map[POS]] = false;
        int old_cost = current_cost[vertex_map[POS]];
        graphs::MeanPayoffGraph::vertex_descriptor best_successor =
            boost::graph_traits<graphs::MeanPayoffGraph>::null_vertex();

        if (graph[POS].player) {
            // Player 1 minimizer
            const auto [out_edges_begin, out_edges_end] = boost::out_edges(POS, graph);
            for (const auto &edge : boost::make_iterator_range(out_edges_begin, out_edges_end)) {
                const auto SUCCESSOR = boost::target(edge, graph);
                if (best_successor == boost::graph_traits<graphs::MeanPayoffGraph>::null_vertex()) {
                    best_successor = SUCCESSOR;
                    current_count[vertex_map[POS]] = 1;
                } else {
                    // Compare with current best successor
                    if (current_cost[vertex_map[best_successor]] > current_cost[vertex_map[SUCCESSOR]]) {
                        best_successor = SUCCESSOR;
                        current_count[vertex_map[POS]] = 1;
                    } else if (current_cost[vertex_map[best_successor]] == current_cost[vertex_map[SUCCESSOR]]) {
                        current_count[vertex_map[POS]]++;
                    }
                }
            }

            if (current_cost[vertex_map[best_successor]] >= limit) {
                current_count[vertex_map[POS]] = 0;
                lifts++;
                current_cost[vertex_map[POS]] = limit;
            } else {
                int sum = current_cost[vertex_map[best_successor]] + graph[POS].weight;
                if (sum >= limit) {
                    sum = limit;
                }
                if (current_cost[vertex_map[POS]] < sum) {
                    lifts++;
                    current_cost[vertex_map[POS]] = sum;
                }
            }
        } else {
            // Player 0 maximizer
            const auto [out_edges_begin, out_edges_end] = boost::out_edges(POS, graph);
            for (const auto &edge : boost::make_iterator_range(out_edges_begin, out_edges_end)) {
                const auto SUCCESSOR = boost::target(edge, graph);
                if (best_successor == boost::graph_traits<graphs::MeanPayoffGraph>::null_vertex()) {
                    best_successor = SUCCESSOR;
                } else {
                    if (current_cost[vertex_map[best_successor]] < current_cost[vertex_map[SUCCESSOR]]) {
                        best_successor = SUCCESSOR;
                    }
                }
            }

            if (current_cost[vertex_map[best_successor]] >= limit) {
                lifts++;
                current_cost[vertex_map[POS]] = limit;
                current_strategy[vertex_map[POS]] = best_successor;
            } else {
                int sum = current_cost[vertex_map[best_successor]] + graph[POS].weight;
                if (sum >= limit) {
                    sum = limit;
                }
                if (current_cost[vertex_map[POS]] < sum) {
                    lifts++;
                    current_cost[vertex_map[POS]] = sum;
                    current_strategy[vertex_map[POS]] = best_successor;
                }
            }
        }

        // Process predecessors by manually finding incoming edges
        const auto [all_vertices_begin, all_vertices_end] = boost::vertices(graph);
        for (const auto &other_vertex : boost::make_iterator_range(all_vertices_begin, all_vertices_end)) {
            const auto [out_edges_begin, out_edges_end] = boost::out_edges(other_vertex, graph);
            for (const auto &edge : boost::make_iterator_range(out_edges_begin, out_edges_end)) {
                if (boost::target(edge, graph) == POS) {
                    // Found a predecessor
                    const auto PREDECESSOR = other_vertex;
                    if (!b_atr[vertex_map[PREDECESSOR]] &&
                        (current_cost[vertex_map[PREDECESSOR]] < limit) &&
                        ((current_cost[vertex_map[POS]] == limit) ||
                         (current_cost[vertex_map[PREDECESSOR]] <
                          current_cost[vertex_map[POS]] + graph[PREDECESSOR].weight))) {

                        if (graph[PREDECESSOR].player) {
                            if (current_cost[vertex_map[PREDECESSOR]] >= old_cost + graph[PREDECESSOR].weight) {
                                current_count[vertex_map[PREDECESSOR]]--;
                            }
                            if (current_count[vertex_map[PREDECESSOR]] <= 0) {
                                t_atr.push(PREDECESSOR);
                                b_atr[vertex_map[PREDECESSOR]] = true;
                            }
                        } else {
                            t_atr.push(PREDECESSOR);
                            b_atr[vertex_map[PREDECESSOR]] = true;
                        }
                    }
                }
            }
        }
    }

    // Set the final solution
    for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
        // Set the quantitative value (currentCost as int)
        solution.set_value(vertex, current_cost[vertex_map[vertex]]);

        if (current_cost[vertex_map[vertex]] >= limit) {
            solution.set_winning_player(vertex, 0);
        } else {
            solution.set_winning_player(vertex, 1);

            if (graph[vertex].player) {
                const auto [out_edges_begin, out_edges_end] = boost::out_edges(vertex, graph);
                for (const auto &edge : boost::make_iterator_range(out_edges_begin, out_edges_end)) {
                    const auto SUCCESSOR = boost::target(edge, graph);
                    if (current_cost[vertex_map[SUCCESSOR]] == 0 ||
                        current_cost[vertex_map[vertex]] >=
                            current_cost[vertex_map[SUCCESSOR]] + graph[vertex].weight) {
                        current_strategy[vertex_map[vertex]] = SUCCESSOR;
                        break;
                    }
                }
            }
        }

        // Only set strategy if it's a valid vertex
        if (current_strategy[vertex_map[vertex]] != boost::graph_traits<graphs::MeanPayoffGraph>::null_vertex()) {
            solution.set_strategy(vertex, current_strategy[vertex_map[vertex]]);
        }
    }

    // Game finished, log trace and return solution
    LGG_TRACE("Solved with ", iterations, " iterations");
    LGG_TRACE("Solved with ", lifts, " lifts");
    solution.set_solved(true);
    return solution;
}

} // namespace solvers
} // namespace ggg
