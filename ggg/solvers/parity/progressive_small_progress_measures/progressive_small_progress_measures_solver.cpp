#include "progressive_small_progress_measures_solver.hpp"
#include "libggg/graphs/priority_utilities.hpp"
#include <boost/graph/graph_traits.hpp>
#include <cassert>
#include <queue>
#include <vector>

namespace ggg {
namespace solvers {

RSSolution<graphs::ParityGraph> ProgressiveSmallProgressMeasuresSolver::solve(const graphs::ParityGraph &graph) {
    // Initialize algorithm data structures and state
    init(graph);

    // Initialize all progress measures to 0 (bottom element)
    for (int i = 0; i < k * boost::num_vertices(*pv); i++) {
        pms[i] = 0;
    }

    // Initialize all strategies to undefined (-1)
    for (int i = 0; i < boost::num_vertices(*pv); i++) {
        strategy[i] = -1;
    }

    // Initialize priority counts to zero
    for (int i = 0; i < k; i++) {
        counts[i] = 0;
    }

    // Count vertices at each priority level (needed for progress measure bounds)
    auto vertices = boost::vertices(*pv);
    for (auto vertex_it = vertices.first; vertex_it != vertices.second; ++vertex_it) {
        auto vertex = *vertex_it;
        int node = vertex_to_node(*pv, vertex);
        counts[(*pv)[vertex].priority]++;
    }

    // Initialize dirty flags to clean state
    for (int n = 0; n < boost::num_vertices(*pv); n++) {
        dirty[n] = 0;
    }

    // Reset performance counters
    lift_count = lift_attempt = 0;

    /**
     * Initialization phase: Process vertices in reverse order to establish
     * initial progress measures and propagate changes backwards through the graph
     */
    for (int n = boost::num_vertices(*pv) - 1; n >= 0; n--) {
        if (lift(n, -1)) {
            auto vertex = node_to_vertex(*pv, n);

            // Find all predecessors of this vertex and attempt to lift them
            // Note: We manually iterate since Boost doesn't provide in_edges for all graph types
            auto all_vertices = boost::vertices(*pv);
            for (auto other_vertex_it = all_vertices.first; other_vertex_it != all_vertices.second; ++other_vertex_it) {
                auto other_vertex = *other_vertex_it;
                auto outgoing_edges = boost::out_edges(other_vertex, *pv);
                for (auto edge_it = outgoing_edges.first; edge_it != outgoing_edges.second; ++edge_it) {
                    auto edge = *edge_it;
                    if (boost::target(edge, *pv) == vertex) {
                        int from_node = vertex_to_node(*pv, other_vertex);
                        if (lift(from_node, n)) {
                            todo_push(from_node);
                        }
                    }
                }
            }
        }
    }

    /**
     * Main iteration phase: Process work queue until no more improvements possible
     * Periodically run global update phases to handle complex dependencies
     */
    int64_t last_update = 0;

    while (!todo.empty()) {
        int n = todo_pop();
        auto vertex = node_to_vertex(*pv, n);

        // Process all predecessors of the current vertex
        for (auto vertex_it = vertices.first; vertex_it != vertices.second; ++vertex_it) {
            auto other_vertex = *vertex_it;
            auto outgoing_edges = boost::out_edges(other_vertex, *pv);
            for (auto edge_it = outgoing_edges.first; edge_it != outgoing_edges.second; ++edge_it) {
                auto edge = *edge_it;
                if (boost::target(edge, *pv) == vertex) {
                    int from_node = vertex_to_node(*pv, other_vertex);
                    if (lift(from_node, n)) {
                        todo_push(from_node);
                    }
                }
            }
        }

        // Periodically run global update phases to ensure convergence
        if (last_update + 10 * boost::num_vertices(*pv) < lift_count) {
            last_update = lift_count;
            update(0); // Update for player 0
            update(1); // Update for player 1
        }
    }

    // Extract solution from final progress measures
    RSSolution<graphs::ParityGraph> solution;
    solution.set_solved(true);

    for (auto vertex_it = vertices.first; vertex_it != vertices.second; ++vertex_it) {
        auto vertex = *vertex_it;
        int node = vertex_to_node(*pv, vertex);
        int *pm = pms.data() + k * node;

        // Check for invalid state (both players have same Top status)
        if ((pm[0] == -1) == (pm[1] == -1)) {
            solution.set_solved(false);
        }

        // In the progress measures algorithm, each player has a progress measure at each vertex.
        // The special value "Top" (encoded as -1 in this implementation) indicates that the player can force a win from this vertex.
        // By construction, only one player's measure can be Top at a given vertex.
        // If pm[0] == -1, then player 0 can force a win from this vertex; otherwise, player 1 can.
        int winner = pm[0] == -1 ? 0 : 1;
        solution.set_winning_player(vertex, winner);

        // Set strategy for winning vertices where owner matches winner
        if ((*pv)[vertex].player == winner && strategy[node] != -1) {
            graphs::ParityVertex strategy_vertex = node_to_vertex(*pv, strategy[node]);
            solution.set_strategy(vertex, strategy_vertex);
        }
    }

    return solution;
}

void ProgressiveSmallProgressMeasuresSolver::init(const graphs::ParityGraph &game) {
    // Store pointer to the game for later access
    pv = &game;

    // Determine k = highest priority + 1 (must be at least 2 for algorithm correctness)
    k = graphs::priority_utilities::get_max_priority(game) + 1;
    if (k < 2)
        k = 2;

    // Allocate storage for algorithm state
    int vertex_count = boost::num_vertices(game);
    pms.resize(k * vertex_count);  // Progress measures: k values per vertex
    strategy.resize(vertex_count); // Strategy function
    counts.resize(k);              // Count of vertices at each priority
    tmp.resize(k);                 // Temporary storage for progress measure operations
    best.resize(k);                // Best progress measure found during comparisons
    dirty.resize(vertex_count);    // Dirty flags for work queue management
    unstable.resize(vertex_count); // Instability flags for global updates
}

bool ProgressiveSmallProgressMeasuresSolver::pm_less(int *a, int *b, int d, int pl) {
    // Handle Top values: Top is greater than any finite measure
    if (b[pl] == -1)
        return a[pl] != -1; // a < b only if a is not Top but b is Top
    else if (a[pl] == -1)
        return false; // a >= b if a is Top but b is not

    // Lexicographic comparison starting from highest priority with player's parity
    const int start = ((k & 1) == pl) ? k - 2 : k - 1;
    for (int i = start; i >= d; i -= 2) {
        if (a[i] == b[i])
            continue; // Equal components, check next priority

        // Both values exceed count bounds - treat as equivalent (both "too large")
        if (a[i] > counts[i] && b[i] > counts[i])
            return false;

        return a[i] < b[i];
    }
    return false; // All compared components equal
}

void ProgressiveSmallProgressMeasuresSolver::pm_copy(int *dst, int *src, int pl) {
    // Copy values at priorities with player pl's parity (pl, pl+2, pl+4, ...)
    for (int i = pl; i < k; i += 2) {
        dst[i] = src[i];
    }
}

void ProgressiveSmallProgressMeasuresSolver::prog(int *dst, int *src, int d, int pl) {
    // If source is already Top, destination is also Top
    if (src[pl] == -1) {
        dst[pl] = -1;
        return;
    }

    // Reset all priorities lower than d to 0
    int i = pl;
    for (; i < d; i += 2) {
        dst[i] = 0;
    }

    // Increment at priority d only if it has player pl's parity
    int carry = (d % 2 == pl) ? 1 : 0;

    // Process priorities from d upwards with carry propagation
    for (; i < k; i += 2) {
        int v = src[i] + carry;
        if (v > counts[i]) {
            // Overflow: reset to 0 and carry to next priority
            dst[i] = 0;
            carry = 1;
        } else {
            // No overflow: store value and stop carrying
            dst[i] = v;
            carry = 0;
        }
    }

    // If final carry remains, the measure becomes Top
    if (carry)
        dst[pl] = -1;
}

bool ProgressiveSmallProgressMeasuresSolver::canlift(int node, int pl) {
    // Get pointer to current progress measure for this node
    int *pm = pms.data() + k * node;

    // Cannot lift if already Top
    if (pm[pl] == -1)
        return false;

    auto vertex = node_to_vertex(*pv, node);
    const int d = (*pv)[vertex].priority;

    if ((*pv)[vertex].player == pl) {
        // Maximizing player: check if any successor offers improvement
        auto outgoing_edges = boost::out_edges(vertex, *pv);
        for (auto edge_it = outgoing_edges.first; edge_it != outgoing_edges.second; ++edge_it) {
            auto edge = *edge_it;
            auto to_vertex = boost::target(edge, *pv);
            int to_node = vertex_to_node(*pv, to_vertex);

            // Compute progress from successor and check if it's better
            prog(tmp.data(), pms.data() + k * to_node, d, pl);
            if (pm_less(pm, tmp.data(), d, pl))
                return true;
        }
        return false;
    } else {
        // Minimizing player: find best successor and check if it offers improvement
        int best_to = -1;
        auto outgoing_edges = boost::out_edges(vertex, *pv);
        for (auto edge_it = outgoing_edges.first; edge_it != outgoing_edges.second; ++edge_it) {
            auto edge = *edge_it;
            auto to_vertex = boost::target(edge, *pv);
            int to_node = vertex_to_node(*pv, to_vertex);

            // Compute progress from this successor
            prog(tmp.data(), pms.data() + k * to_node, d, pl);
            if (best_to == -1 || pm_less(tmp.data(), best.data(), d, pl)) {
                // This successor is better than previous best
                for (int i = 0; i < k; i++) {
                    best[i] = tmp[i];
                }
                best_to = to_node;
            }
        }
        if (best_to == -1)
            return false;
        return pm_less(pm, best.data(), d, pl);
    }
}

bool ProgressiveSmallProgressMeasuresSolver::lift(int node, int target) {
    // Get pointer to current progress measure for this node
    int *pm = pms.data() + k * node;

    // Cannot lift if both players already have Top measures
    if (pm[0] == -1 && pm[1] == -1)
        return false;

    auto vertex = node_to_vertex(*pv, node);
    const int pl_max = (*pv)[vertex].player; // Maximizing player (vertex owner)
    const int pl_min = 1 - pl_max;           // Minimizing player (opponent)
    const int d = (*pv)[vertex].priority;    // Vertex priority

    int best_ch0 = -1, best_ch1 = -1; // Track which measures changed

    // Lift for maximizing player (vertex owner)
    if (pm[pl_max] != -1) {
        if (target != -1) {
            // Consider only the specified target successor
            prog(tmp.data(), pms.data() + k * target, d, pl_max);
            if (pm_less(pm, tmp.data(), d, pl_max)) {
                pm_copy(pm, tmp.data(), pl_max);
                if (pl_max)
                    best_ch1 = target;
                else
                    best_ch0 = target;
            }
        } else {
            // Consider all successors and take maximum
            auto outgoing_edges = boost::out_edges(vertex, *pv);
            for (auto edge_it = outgoing_edges.first; edge_it != outgoing_edges.second; ++edge_it) {
                auto edge = *edge_it;
                auto to_vertex = boost::target(edge, *pv);
                int to_node = vertex_to_node(*pv, to_vertex);
                prog(tmp.data(), pms.data() + k * to_node, d, pl_max);
                if (pm_less(pm, tmp.data(), d, pl_max)) {
                    pm_copy(pm, tmp.data(), pl_max);
                    if (pl_max)
                        best_ch1 = to_node;
                    else
                        best_ch0 = to_node;
                }
            }
        }
    }

    // Lift for minimizing player (opponent)
    if (pm[pl_min] != -1 && (target == -1 || target == strategy[node])) {
        int best_to = -1;

        // Find the minimum among all successors
        auto outgoing_edges = boost::out_edges(vertex, *pv);
        for (auto edge_it = outgoing_edges.first; edge_it != outgoing_edges.second; ++edge_it) {
            auto edge = *edge_it;
            auto to_vertex = boost::target(edge, *pv);
            int to_node = vertex_to_node(*pv, to_vertex);
            prog(tmp.data(), pms.data() + k * to_node, d, pl_min);
            if (best_to == -1 || pm_less(tmp.data(), best.data(), d, pl_min)) {
                for (int i = 0; i < k; i++) {
                    best[i] = tmp[i];
                }
                best_to = to_node;
            }
        }

        // Update strategy (even if measure doesn't change, strategy might)
        strategy[node] = best_to;

        // Update progress measure if improvement found
        if (pm_less(pm, best.data(), d, pl_min)) {
            pm_copy(pm, best.data(), pl_min);
            if (pl_min)
                best_ch1 = best_to;
            else
                best_ch0 = best_to;
        }
    }

    bool ch0 = best_ch0 != -1;
    bool ch1 = best_ch1 != -1;

    // Update priority counts when measures become Top
    // Decrease count if priority matches winner and measure becomes Top
    if (ch0 && pm[0] == -1) {
        if ((d & 1) == 0) // Priority has even parity (player 0)
            counts[d]--;
    }
    if (ch1 && pm[1] == -1) {
        if ((d & 1) == 1) // Priority has odd parity (player 1)
            counts[d]--;
    }

    // Update performance counters and return result
    if (ch0 || ch1) {
        lift_count++; // Successful lift
        return true;
    }

    lift_attempt++; // Unsuccessful lift attempt
    return false;
}

void ProgressiveSmallProgressMeasuresSolver::update(int pl) {
    std::queue<int> q;

    // Phase 1: Identify initially unstable nodes
    // A node is unstable if its measure is Top or can be lifted
    for (int i = 0; i < boost::num_vertices(*pv); i++) {
        unstable[i] = 0; // Initialize as stable
        if (pms[k * i + pl] == -1 || canlift(i, pl)) {
            unstable[i] = 1;
            q.push(i);
        }
    }

    auto vertices = boost::vertices(*pv);

    // Phase 2: Propagate instability backwards through the graph
    while (!q.empty()) {
        int n = q.front();
        q.pop();
        auto vertex = node_to_vertex(*pv, n);

        // Check all predecessors of this unstable node
        for (auto vertex_it = vertices.first; vertex_it != vertices.second; ++vertex_it) {
            auto other_vertex = *vertex_it;
            auto outgoing_edges = boost::out_edges(other_vertex, *pv);
            for (auto edge_it = outgoing_edges.first; edge_it != outgoing_edges.second; ++edge_it) {
                auto edge = *edge_it;
                if (boost::target(edge, *pv) == vertex) {
                    int m = vertex_to_node(*pv, other_vertex);
                    if (unstable[m])
                        continue; // Already marked unstable

                    // For opponent nodes, check if all stable successors force instability
                    if ((*pv)[other_vertex].player != pl) {
                        int best_to = -1;
                        const int d = (*pv)[other_vertex].priority;
                        auto out_edges = boost::out_edges(other_vertex, *pv);

                        // Find best stable successor
                        for (auto edge_it = out_edges.first; edge_it != out_edges.second; ++edge_it) {
                            auto out_edge = *edge_it;
                            auto to_vertex = boost::target(out_edge, *pv);
                            int to_node = vertex_to_node(*pv, to_vertex);
                            if (unstable[to_node])
                                continue; // Skip unstable successors

                            prog(tmp.data(), pms.data() + k * to_node, d, pl);
                            if (best_to == -1 || pm_less(tmp.data(), best.data(), d, pl)) {
                                for (int i = 0; i < k; i++) {
                                    best[i] = tmp[i];
                                }
                                best_to = to_node;
                            }
                        }

                        if (best_to == -1)
                            continue; // No stable successors

                        // If current measure is not worse than best stable successor,
                        // this node doesn't become unstable
                        if (pm_less(pms.data() + k * m, best.data(), d, pl))
                            continue;

                        // Mark as unstable and add to queue
                        unstable[m] = 1;
                        q.push(m);
                    }
                }
            }
        }
    }

    // Phase 3: Set measures to Top for stable nodes of the opponent
    // If a node is stable for player pl but has finite measure for opponent,
    // and its priority doesn't favor the opponent, set opponent's measure to Top
    for (int i = 0; i < boost::num_vertices(*pv); i++) {
        if (unstable[i] == 0 && pms[k * i + 1 - pl] != -1) {
            auto vertex = node_to_vertex(*pv, i);
            if (((*pv)[vertex].priority & 1) != pl) {
                // Priority doesn't favor player pl, so opponent can't win from here
                counts[(*pv)[vertex].priority]--;
                pms[k * i + 1 - pl] = -1;
                todo_push(i); // Schedule for further processing
            }
        }
    }
}

void ProgressiveSmallProgressMeasuresSolver::todo_push(int node) {
    if (dirty[node] == 0) {
        dirty[node] = 1; // Mark as dirty (in queue)
        todo.push(node); // Add to work queue
    }
}

int ProgressiveSmallProgressMeasuresSolver::todo_pop() {
    int node = todo.front();
    todo.pop();
    dirty[node] = 0; // Clear dirty flag
    return node;
}

int ProgressiveSmallProgressMeasuresSolver::vertex_to_node(const graphs::ParityGraph &game, graphs::ParityVertex vertex) {
    // Simple linear mapping - assumes vertices are numbered 0 to n-1
    auto vertices = boost::vertices(game);
    int index = 0;
    for (auto it = vertices.first; it != vertices.second; ++it) {
        if (*it == vertex)
            return index;
        index++;
    }
    return -1; // Should not happen in well-formed graphs
}

graphs::ParityVertex ProgressiveSmallProgressMeasuresSolver::node_to_vertex(const graphs::ParityGraph &game, int node) {
    // Simple linear mapping - assumes vertices are numbered 0 to n-1
    auto vertices = boost::vertices(game);
    int index = 0;
    for (auto it = vertices.first; it != vertices.second; ++it) {
        if (index == node)
            return *it;
        index++;
    }
    return graphs::ParityVertex(); // Should not happen in well-formed graphs
}

} // namespace solvers
} // namespace ggg