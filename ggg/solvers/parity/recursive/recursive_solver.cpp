#include "recursive_solver.hpp"
#include "libggg/graphs/player_utilities.hpp"
#include "libggg/graphs/priority_utilities.hpp"
#include "libggg/utils/logging.hpp"
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <algorithm>
#include <stdexcept>

namespace ggg {
namespace solvers {

RecursiveParitySolver::RecursiveParitySolver()
    : max_recursion_depth_(0), // 0 means unlimited
      enable_statistics_(true),
      current_depth_(0),
      max_reached_depth_(0),
      subgames_created_(0) {
    // Pre-allocate some capacity for working sets to reduce allocations
    working_set_.clear();
    vertex_mapping_.clear();
}

RecursiveParitySolver::RecursiveParitySolver(size_t max_depth)
    : max_recursion_depth_(max_depth),
      enable_statistics_(true),
      current_depth_(0),
      max_reached_depth_(0),
      subgames_created_(0) {
    // Pre-allocate some capacity for working sets
    working_set_.clear();
    vertex_mapping_.clear();
}

void RecursiveParitySolver::reset_solve_state() {
    current_depth_ = 0;
    max_reached_depth_ = 0;
    subgames_created_ = 0;
    working_set_.clear();
    vertex_mapping_.clear();
}

RecursiveParitySolution RecursiveParitySolver::solve(const graphs::ParityGraph &graph) {
    reset_solve_state();
    LGG_TRACE("Starting recursive solve with ", boost::num_vertices(graph), " vertices");
    auto solution = solve_internal(graph, 0);

    // Set the statistics in the solution
    solution.set_max_depth_reached(max_reached_depth_);
    solution.set_subgames_created(subgames_created_);

    return solution;
}

RecursiveParitySolution RecursiveParitySolver::solve_internal(const graphs::ParityGraph &graph, size_t depth) {
    // Update depth tracking
    current_depth_ = depth;
    if (enable_statistics_ && depth > max_reached_depth_) {
        max_reached_depth_ = depth;
    }

    // Check recursion depth limit
    if (max_recursion_depth_ > 0 && depth >= max_recursion_depth_) {
        throw std::runtime_error("Maximum recursion depth exceeded: " + std::to_string(max_recursion_depth_));
    }

    LGG_TRACE("Recursive solve at depth ", depth, " with ", boost::num_vertices(graph), " vertices");
    RecursiveParitySolution solution;

    // Base case: empty game
    if (boost::num_vertices(graph) == 0) {
        LGG_TRACE("Empty game - returning solved");
        solution.set_solved(true);
        return solution;
    }

    // Find highest priority
    int max_priority = graphs::priority_utilities::get_max_priority(graph);
    int priority_player = (max_priority % 2 == 0) ? 0 : 1;

    LGG_TRACE("Max priority: ", max_priority, " (player ", priority_player, ")");

    // Get vertices with max priority
    auto max_priority_vertices = graphs::priority_utilities::get_vertices_with_priority(graph, max_priority);

    // Reuse the working set to avoid allocations
    working_set_.clear();
    working_set_.insert(max_priority_vertices.begin(), max_priority_vertices.end());

    LGG_TRACE("Found ", working_set_.size(), " vertices with max priority");

    // Compute attractor for priority player
    auto [attractorSet, attractorStrategy] = graphs::player_utilities::compute_attractor(graph, working_set_, priority_player);

    // Mark attractor vertices as won by priority player
    for (auto vertex : attractorSet) {
        solution.set_winning_player(vertex, priority_player);
    }

    // Add attractor strategy to solution
    for (auto [from, to] : attractorStrategy) {
        solution.set_strategy(from, to);
    }

    // Create subgame without attractor
    if (enable_statistics_) {
        subgames_created_++;
    }
    auto subgame = create_subgame(graph, attractorSet);

    // Recursively solve subgame with incremented depth
    auto sub_solution = solve_internal(subgame, depth + 1);

    if (!sub_solution.is_solved()) {
        solution.set_solved(false);
        return solution;
    }

    // Check if opponent can win back any vertices
    working_set_.clear(); // Reuse for opponent wins
    const auto &sub_winning_regions = sub_solution.get_winning_regions();

    // Need to create vertex mapping from subgame back to original
    // Reuse the vertex_mapping_ member to avoid allocation
    vertex_mapping_.clear();
    const auto [vertices_begin, vertices_end] = boost::vertices(graph);
    const auto [sub_vertices_begin, sub_vertices_end] = boost::vertices(subgame);

    auto sub_it = sub_vertices_begin;
    for (auto it = vertices_begin; it != vertices_end; ++it) {
        if (attractorSet.find(*it) == attractorSet.end()) { // Not in attractor
            if (sub_it != sub_vertices_end) {
                vertex_mapping_[*sub_it] = *it;
                ++sub_it;
            }
        }
    }

    for (auto [subgame_vertex, player] : sub_winning_regions) {
        if (player == 1 - priority_player) {
            auto original_vertex_it = vertex_mapping_.find(subgame_vertex);
            if (original_vertex_it != vertex_mapping_.end()) {
                working_set_.insert(original_vertex_it->second);
            }
        }
    }

    if (!working_set_.empty()) {
        // If the opponent can win some vertices in the subgame, the compute opponent attractor
        // on the original graph

        auto [opponentAttractor, opponentStrategy] = graphs::player_utilities::compute_attractor(graph, working_set_, 1 - priority_player);

        // Mark opponent attractor vertices
        for (auto vertex : opponentAttractor) {
            solution.set_winning_player(vertex, 1 - priority_player);
        }

        // Add opponent strategy
        for (auto [from, to] : opponentStrategy) {
            solution.set_strategy(from, to);
        }

        // Recursively solve game without opponent attractor (not continuing with remaining subgame)
        if (enable_statistics_) {
            subgames_created_++;
        }
        auto final_subgame = create_subgame(graph, opponentAttractor);
        auto final_solution = solve_internal(final_subgame, depth + 1);

        if (!final_solution.is_solved()) {
            solution.set_solved(false);
            return solution;
        }

        // Merge final solution with proper vertex mapping
        // Create a temporary mapping for the final subgame
        std::map<graphs::ParityVertex, graphs::ParityVertex> final_vertex_mapping;
        const auto [final_vertices_begin, final_vertices_end] = boost::vertices(final_subgame);

        auto final_it = final_vertices_begin;
        for (auto it = vertices_begin; it != vertices_end; ++it) {
            if (opponentAttractor.find(*it) == opponentAttractor.end()) { // Not in opponent attractor
                if (final_it != final_vertices_end) {
                    final_vertex_mapping[*final_it] = *it;
                    ++final_it;
                }
            }
        }

        const auto &final_winning_regions = final_solution.get_winning_regions();
        for (auto [subgame_vertex, player] : final_winning_regions) {
            auto original_vertex_it = final_vertex_mapping.find(subgame_vertex);
            if (original_vertex_it != final_vertex_mapping.end()) {
                solution.set_winning_player(original_vertex_it->second, player);
            }
        }
        const auto &final_strategies = final_solution.get_strategies();
        for (auto [subgame_from, subgame_to] : final_strategies) {
            auto original_from_it = final_vertex_mapping.find(subgame_from);
            auto original_to_it = final_vertex_mapping.find(subgame_to);
            if (original_from_it != final_vertex_mapping.end() &&
                original_to_it != final_vertex_mapping.end()) {
                solution.set_strategy(original_from_it->second, original_to_it->second);
            }
        }
    } else {
        // Priority player wins all remaining vertices - merge subgame solution properly
        for (auto [subgame_vertex, player] : sub_winning_regions) {
            auto original_vertex_it = vertex_mapping_.find(subgame_vertex);
            if (original_vertex_it != vertex_mapping_.end()) {
                solution.set_winning_player(original_vertex_it->second, player);
            }
        }
        const auto &sub_strategies = sub_solution.get_strategies();
        for (auto [subgame_from, subgame_to] : sub_strategies) {
            auto original_from_it = vertex_mapping_.find(subgame_from);
            auto original_to_it = vertex_mapping_.find(subgame_to);
            if (original_from_it != vertex_mapping_.end() &&
                original_to_it != vertex_mapping_.end()) {
                solution.set_strategy(original_from_it->second, original_to_it->second);
            }
        }
    }

    // Final step: Remove strategies for vertices where owner != winning player
    // Create a new solution with properly filtered strategies
    RecursiveParitySolution filtered_solution(true);

    // Copy winning regions
    const auto &winning_regions = solution.get_winning_regions();
    for (auto [vertex, winning_player] : winning_regions) {
        filtered_solution.set_winning_player(vertex, winning_player);
    }

    // Copy strategies only for vertices where owner == winning player
    const auto &strategies = solution.get_strategies();
    for (auto [from_vertex, to_vertex] : strategies) {
        int vertex_owner = graph[from_vertex].player;
        int winning_player = solution.get_winning_player(from_vertex);
        if (vertex_owner == winning_player) {
            filtered_solution.set_strategy(from_vertex, to_vertex);
        }
    }

    // Fill in missing strategies: for each vertex owned by winning player without strategy,
    // choose any edge that goes to a vertex also won by the same player
    const auto [fill_vertices_begin, fill_vertices_end] = boost::vertices(graph);
    for (auto it = fill_vertices_begin; it != fill_vertices_end; ++it) {
        auto vertex = *it;
        int vertex_owner = graph[vertex].player;
        int winning_player = filtered_solution.get_winning_player(vertex);

        // If this vertex is owned by the winning player but has no strategy set
        if (vertex_owner == winning_player &&
            filtered_solution.get_strategy(vertex) == boost::graph_traits<graphs::ParityGraph>::null_vertex()) {
            // Find any outgoing edge to a vertex also won by the same player
            const auto [out_edges_begin, out_edges_end] = boost::out_edges(vertex, graph);
            for (auto edge_it = out_edges_begin; edge_it != out_edges_end; ++edge_it) {
                auto target = boost::target(*edge_it, graph);
                int target_winning_player = filtered_solution.get_winning_player(target);
                if (target_winning_player == winning_player) {
                    filtered_solution.set_strategy(vertex, target);
                    break; // Found a valid strategy, move to next vertex
                }
            }
        }
    }

    filtered_solution.set_solved(true);
    return filtered_solution;
}

graphs::ParityGraph RecursiveParitySolver::create_subgame(const graphs::ParityGraph &graph,
                                                          const std::set<graphs::ParityVertex> &to_remove) {
    graphs::ParityGraph subgame;
    std::map<graphs::ParityVertex, graphs::ParityVertex> vertex_mapping;

    // Copy vertices not in to_remove
    const auto [vertices_begin, vertices_end] = boost::vertices(graph);
    std::for_each(vertices_begin, vertices_end, [&](const auto &vertex) {
        if (!to_remove.count(vertex)) {
            const auto &name = graph[vertex].name;
            const auto PLAYER = graph[vertex].player;
            const auto PRIORITY = graph[vertex].priority;
            const auto NEW_VERTEX = graphs::add_vertex(subgame, name, PLAYER, PRIORITY);
            vertex_mapping[vertex] = NEW_VERTEX;
        }
    });

    // Copy edges between remaining vertices
    const auto [edges_begin, edges_end] = boost::edges(graph);
    std::for_each(edges_begin, edges_end, [&](const auto &edge) {
        const auto SOURCE = boost::source(edge, graph);
        const auto TARGET = boost::target(edge, graph);

        if (!to_remove.count(SOURCE) && !to_remove.count(TARGET)) {
            const auto &label = graph[edge].label;
            // Find corresponding vertices in subgame and add edge
            const auto NEW_SOURCE = vertex_mapping[SOURCE];
            const auto NEW_TARGET = vertex_mapping[TARGET];
            graphs::add_edge(subgame, NEW_SOURCE, NEW_TARGET, label);
        }
    });

    return subgame;
}

void RecursiveParitySolver::merge_solutions(RecursiveParitySolution &original_solution,
                                            const RecursiveParitySolution &subgame_solution,
                                            const std::map<graphs::ParityVertex, graphs::ParityVertex> &vertex_mapping,
                                            const graphs::ParityGraph &graph) {
    // Merge subgame solution back to original
    const auto &sub_winning_regions = subgame_solution.get_winning_regions();
    for (const auto &[subVertex, player] : sub_winning_regions) {
        const auto IT = vertex_mapping.find(subVertex);
        if (IT != vertex_mapping.end()) {
            original_solution.set_winning_player(IT->second, player);
        }
    }

    const auto &sub_strategies = subgame_solution.get_strategies();
    for (const auto &[subFrom, subTo] : sub_strategies) {
        const auto FROM_IT = vertex_mapping.find(subFrom);
        const auto TO_IT = vertex_mapping.find(subTo);
        if (FROM_IT != vertex_mapping.end() && TO_IT != vertex_mapping.end()) {
            original_solution.set_strategy(FROM_IT->second, TO_IT->second);
        }
    }
}

} // namespace solvers
} // namespace ggg