#include "buchi_solver.hpp"
#include "libggg/graphs/player_utilities.hpp"
#include "libggg/graphs/priority_utilities.hpp"
#include "libggg/utils/logging.hpp"
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <algorithm>
#include <map>
#include <optional>
#include <set>

namespace ggg::solvers {

RSSolution<graphs::ParityGraph> BuchiSolver::solve(const graphs::ParityGraph &graph) {

    LGG_DEBUG("Buchi solver starting with ", boost::num_vertices(graph), " vertices");
    RSSolution<graphs::ParityGraph> solution;

    // Validate input
    if (!validate_buchi_game(graph)) {
        LGG_ERROR("Invalid Buchi game: priorities must be 0 or 1"); // TODO swap current parity validation with buchi one
        solution.set_solved(true);
        return solution;
    }

    // Base case: empty game
    if (boost::num_vertices(graph) == 0) {
        LGG_TRACE("Empty game - returning solved");
        solution.set_solved(true);
        return solution;
    }

    // Reset counters
    iterations = 0;
    attractions = 0;

    // Get all vertices and Buchi accepting vertices (priority 1)
    const auto [vertices_begin, vertices_end] = boost::vertices(graph);
    std::set<graphs::ParityVertex> current_active(vertices_begin, vertices_end);
    const auto TARGET_VERTICES = get_buchi_accepting_vertices(graph);

    LGG_TRACE("Found ", TARGET_VERTICES.size(), " Buchi accepting vertices (priority 1)");

    // Main algorithm loop
    while (!current_active.empty()) {
        iterations++; // Increments iteration counter
        // Step 1: Find attractor of Player 1 to target vertices
        std::set<graphs::ParityVertex> p1_attractor = compute_attractor(graph, current_active, 1, TARGET_VERTICES);

        LGG_TRACE("Player 1 attractor to targets has ", p1_attractor.size(), " vertices");

        // Step 2: The complement becomes Player 0's target
        std::set<graphs::ParityVertex> p0_target = compute_complement(current_active, p1_attractor);

        // If no complement exists, Player 1 wins everywhere remaining
        if (p0_target.empty()) {
            LGG_TRACE("No complement - Player 1 wins remaining ", current_active.size(), " vertices");
            // Mark all remaining vertices as winning for Player 1
            for (const auto &curr_active_out : current_active) {
                solution.set_winning_player(curr_active_out, 1);
            }
            break;
        }

        // Step 3: Find Player 0's attractor to the complement
        std::set<graphs::ParityVertex> p0_attractor = compute_attractor(graph, current_active, 0, p0_target);

        LGG_TRACE("Player 0 attractor to complement has ", p0_attractor.size(), " vertices");

        // Step 4: Mark Player 0's attractor as winning for Player 0
        for (const auto &curr_attr_out : p0_attractor) {
            solution.set_winning_player(curr_attr_out, 0);
        }

        // Step 5: Remove Player 0's attractor from the game
        std::set<graphs::ParityVertex> new_active;
        for (const auto &curr_active_out : current_active) {
            if (p0_attractor.find(curr_active_out) == p0_attractor.end()) {
                new_active.insert(curr_active_out);
            }
        }

        current_active = new_active;
    }

    // Compute strategies for both players
    std::map<graphs::ParityVertex, graphs::ParityVertex> strategy;

    // For Player 0 vertices in their winning region, choose strategies
    const auto [all_vertices_begin, all_vertices_end] = boost::vertices(graph);
    for (auto it = all_vertices_begin; it != all_vertices_end; ++it) {
        auto curr_vertex = *it;

        if (solution.get_winning_player(curr_vertex) == 0 && graph[curr_vertex].player == 0) {
            // Player 0 vertex in Player 0's winning region - choose best successor
            const auto [out_edges_begin, out_edges_end] = boost::out_edges(curr_vertex, graph);

            // First, try to find a successor that Player 0 also wins
            for (auto edge_it = out_edges_begin; edge_it != out_edges_end; ++edge_it) {
                auto target = boost::target(*edge_it, graph);
                if (solution.get_winning_player(target) == 0) {
                    strategy[curr_vertex] = target;
                    break;
                }
            }

            // If no winning successor found, take any successor (shouldn't happen in correct algorithm)
            if (strategy.find(curr_vertex) == strategy.end() && out_edges_begin != out_edges_end) {
                strategy[curr_vertex] = boost::target(*out_edges_begin, graph);
            }
        } else if (solution.get_winning_player(curr_vertex) == 1 && graph[curr_vertex].player == 1) {
            // Player 1 vertex in Player 1's winning region - choose best successor
            const auto [out_edges_begin, out_edges_end] = boost::out_edges(curr_vertex, graph);

            // First, try to find a successor that Player 1 also wins
            for (auto edge_it = out_edges_begin; edge_it != out_edges_end; ++edge_it) {
                auto target = boost::target(*edge_it, graph);
                if (solution.get_winning_player(target) == 1) {
                    strategy[curr_vertex] = target;
                    break;
                }
            }

            // If no winning successor found, take any successor
            if (strategy.find(curr_vertex) == strategy.end() && out_edges_begin != out_edges_end) {
                strategy[curr_vertex] = boost::target(*out_edges_begin, graph);
            }
        }
    }

    // Set strategies in the solution
    for (const auto &[from, to] : strategy) {
        solution.set_strategy(from, to);
    }
    // Log results
    const auto PLAYER0_WINS = std::count_if(all_vertices_begin, all_vertices_end,
                                            [&solution](const auto &curr_vertex) {
                                                return solution.get_winning_player(curr_vertex) == 0;
                                            });
    const auto PLAYER1_WINS = std::count_if(all_vertices_begin, all_vertices_end,
                                            [&solution](const auto &curr_vertex) {
                                                return solution.get_winning_player(curr_vertex) == 1;
                                            });

    LGG_DEBUG("Buchi game solved: Player 0 wins ", PLAYER0_WINS, " vertices, Player 1 wins ", PLAYER1_WINS, " vertices");

    LGG_TRACE("Solved with ", iterations, " iterations");
    LGG_TRACE("Solved with ", attractions, " attractions");
    solution.set_solved(true);
    return solution;
}

std::set<graphs::ParityVertex> BuchiSolver::compute_attractor(const graphs::ParityGraph &graph,
                                                              const std::set<graphs::ParityVertex> &active_vertices,
                                                              int curr_player,
                                                              const std::set<graphs::ParityVertex> &curr_target) {

    std::set<graphs::ParityVertex> attractor = curr_target;

    // Only keep target vertices that are actually in active_vertices
    std::set<graphs::ParityVertex> filtered_target;
    std::set_intersection(curr_target.begin(), curr_target.end(),
                          active_vertices.begin(), active_vertices.end(),
                          std::inserter(filtered_target, filtered_target.begin()));
    attractor = filtered_target;

    if (attractor.size() >= active_vertices.size() || attractor.empty()) {
        return attractor;
    }

    bool changed = true;
    while (changed && attractor.size() < active_vertices.size()) {
        changed = false;

        for (const auto &curr_vertex : active_vertices) {
            if (attractor.find(curr_vertex) != attractor.end()) {
                continue; // Already in attractor
            }

            // Count successors that are in the attractor
            int count = 0;
            int total_successors = 0;
            const auto [out_edges_begin, out_edges_end] = boost::out_edges(curr_vertex, graph);

            for (auto edge_it = out_edges_begin; edge_it != out_edges_end; ++edge_it) {
                auto target = boost::target(*edge_it, graph);
                if (active_vertices.find(target) != active_vertices.end()) {
                    total_successors++;
                    if (attractor.find(target) != attractor.end()) {
                        count++;
                    }
                }
            }

            // Add vertex to attractor based on player logic
            if (graph[curr_vertex].player == curr_player && count > 0) {
                // Current player's vertex with at least one successor in attractor
                attractor.insert(curr_vertex);
                attractions++;
                changed = true;
            } else if (graph[curr_vertex].player != curr_player && count == total_successors && total_successors > 0) {
                // Opponent's vertex with all successors in attractor
                attractor.insert(curr_vertex);
                attractions++;
                changed = true;
            }
        }
    }

    return attractor;
}

bool BuchiSolver::validate_buchi_game(const graphs::ParityGraph &graph) const {
    const auto [vertices_begin, vertices_end] = boost::vertices(graph);
    return std::all_of(vertices_begin, vertices_end, [&graph](const auto &curr_vertex) {
        int priority = graph[curr_vertex].priority;
        return priority == 0 || priority == 1;
    });
}

std::set<graphs::ParityVertex> BuchiSolver::get_buchi_accepting_vertices(const graphs::ParityGraph &graph) const {
    auto vertices_vector = graphs::priority_utilities::get_vertices_with_priority(graph, 1);
    return std::set<graphs::ParityVertex>(vertices_vector.begin(), vertices_vector.end());
}

std::set<graphs::ParityVertex> BuchiSolver::compute_complement(const std::set<graphs::ParityVertex> &active_vertices,
                                                               const std::set<graphs::ParityVertex> &inactive_vertices) const {
    std::set<graphs::ParityVertex> complement;

    for (const auto &curr_vertex : active_vertices) {
        if (inactive_vertices.find(curr_vertex) == inactive_vertices.end()) {
            complement.insert(curr_vertex);
        }
    }

    return complement;
}

} // namespace ggg::solvers
