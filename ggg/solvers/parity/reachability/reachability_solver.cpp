#include "reachability_solver.hpp"
#include "libggg/utils/logging.hpp"
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <algorithm>
#include <queue>
#include <set>

namespace ggg {
namespace solvers {

RSSolution<graphs::ParityGraph> ReachabilitySolver::solve(const graphs::ParityGraph &graph) {
    LGG_DEBUG("Reachability solver starting with ", boost::num_vertices(graph), " vertices");
    RSSolution<graphs::ParityGraph> solution;

    // Validate input
    if (!validate_reachability_game(graph)) {
        LGG_ERROR("Invalid reachability game: priorities must be 0 or 1");
        solution.set_solved(true);
        return solution;
    }

    // Base case: empty game
    if (boost::num_vertices(graph) == 0) {
        LGG_TRACE("Empty game - returning solved");
        solution.set_solved(true);
        return solution;
    }

    // Get all vertices and target vertices (priority 1)
    const auto [vertices_begin, vertices_end] = boost::vertices(graph);
    const std::set<graphs::ParityVertex> all_vertices(vertices_begin, vertices_end);
    const auto target_vertices = get_target_vertices(graph);

    LGG_TRACE("Found ", TARGET_VERTICES.size(), " target vertices (priority 1)");

    // Special case: no target vertices means Player 1 wins everywhere
    if (target_vertices.empty()) {
        LGG_DEBUG("No target vertices found - Player 1 wins all vertices");
        for (const auto &vertex : all_vertices) {
            solution.set_winning_player(vertex, 1);
        }
        solution.set_solved(true);
        return solution;
    }

    // Algorithm for reachability games:
    // Compute Attr₀(TARGET_VERTICES) - the attractor of Player 0 to target vertices
    // Player 0 wins all vertices in this attractor, Player 1 wins the rest
    const auto [player0_winning_region, player0_strategy] = compute_attractor(graph, target_vertices, 0);

    LGG_TRACE("Player 0 attractor computed: ", player0_winning_region.size(), " vertices");

    // Set winning regions
    for (const auto &vertex : player0_winning_region) {
        solution.set_winning_player(vertex, 0);
    }

    for (const auto &vertex : all_vertices) {
        if (player0_winning_region.find(vertex) == player0_winning_region.end()) {
            solution.set_winning_player(vertex, 1);
        }
    }

    // Set strategies from attractor computation
    for (const auto &[from, to] : player0_strategy) {
        solution.set_strategy(from, to);
    }

    // For Player 1 vertices outside the attractor, any strategy works (they win anyway)
    for (const auto &vertex : all_vertices) {
        if (player0_winning_region.find(vertex) == player0_winning_region.end() &&
            graph[vertex].player == 1) {
            const auto [out_begin, out_end] = boost::out_edges(vertex, graph);
            if (out_begin != out_end) {
                solution.set_strategy(vertex, boost::target(*out_begin, graph));
            }
        }
    }

    solution.set_solved(true);

    LGG_DEBUG("Reachability game solved: Player 0 wins ", player0_winning_region.size(),
              " vertices, Player 1 wins ", (all_vertices.size() - player0_winning_region.size()), " vertices");

    return solution;
}

bool ReachabilitySolver::validate_reachability_game(const graphs::ParityGraph &graph) const {
    const auto [vertices_begin, vertices_end] = boost::vertices(graph);
    return std::all_of(vertices_begin, vertices_end, [&graph](const auto &vertex) {
        int priority = graph[vertex].priority;
        return priority == 0 || priority == 1;
    });
}

std::pair<std::set<graphs::ParityVertex>, std::map<graphs::ParityVertex, graphs::ParityVertex>>
ReachabilitySolver::compute_attractor(const graphs::ParityGraph &graph,
                                      const std::set<graphs::ParityVertex> &target,
                                      int player) const {
    std::set<graphs::ParityVertex> attractor = target;
    std::map<graphs::ParityVertex, graphs::ParityVertex> strategy;
    std::queue<graphs::ParityVertex> worklist;

    // Pre-compute incoming edges for efficient predecessor lookup
    std::map<graphs::ParityVertex, std::vector<graphs::ParityVertex>> in_edges;
    std::map<graphs::ParityVertex, int> remaining_out_degree;

    const auto [vertices_begin, vertices_end] = boost::vertices(graph);

    // Build incoming edge map and initialize out-degree counters
    for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
        remaining_out_degree[vertex] = boost::out_degree(vertex, graph);

        const auto [out_begin, out_end] = boost::out_edges(vertex, graph);
        for (const auto &edge : boost::make_iterator_range(out_begin, out_end)) {
            const auto target_vertex = boost::target(edge, graph);
            in_edges[target_vertex].push_back(vertex);
        }
    }

    // Initialize worklist with target vertices
    for (const auto &vertex : target) {
        worklist.push(vertex);
    }

    // Compute attractor using backward reachability
    while (!worklist.empty()) {
        const auto current = worklist.front();
        worklist.pop();

        // Process all predecessors of current vertex
        for (const auto &predecessor : in_edges[current]) {
            // Skip if predecessor is already in attractor
            if (attractor.count(predecessor)) {
                continue;
            }

            bool should_add = false;

            if (graph[predecessor].player == player) {
                // Player vertex: add if it has an edge to attractor
                should_add = true;
                strategy[predecessor] = current;
            } else {
                // Opponent vertex: add only if all outgoing edges lead to attractor
                remaining_out_degree[predecessor]--;
                if (remaining_out_degree[predecessor] == 0) {
                    should_add = true;
                    // For opponent vertices, any successor in attractor works as strategy
                    const auto [out_begin, out_end] = boost::out_edges(predecessor, graph);
                    if (out_begin != out_end) {
                        strategy[predecessor] = boost::target(*out_begin, graph);
                    }
                }
            }

            if (should_add) {
                attractor.insert(predecessor);
                worklist.push(predecessor);
            }
        }
    }

    return {attractor, strategy};
}

std::set<graphs::ParityVertex> ReachabilitySolver::get_target_vertices(const graphs::ParityGraph &graph) const {
    std::set<graphs::ParityVertex> target_vertices;
    const auto [vertices_begin, vertices_end] = boost::vertices(graph);

    for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
        if (graph[vertex].priority == 1) {
            target_vertices.insert(vertex);
        }
    }

    return target_vertices;
}

} // namespace solvers
} // namespace ggg
