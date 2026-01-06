#pragma once
#include "libggg/graphs/graph_utilities.hpp"
#include <boost/graph/depth_first_search.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <algorithm>
#include <map>
#include <queue>
#include <set>
#include <stdexcept>

namespace ggg {
namespace graphs {

// --- Define property field lists as macros ---
#define STOCHASTIC_DISCOUNTED_VERTEX_FIELDS(X) \
    X(std::string, name)                       \
    X(int, player)

#define STOCHASTIC_DISCOUNTED_EDGE_FIELDS(X) \
    X(std::string, label)                    \
    X(double, weight)                        \
    X(double, discount)                      \
    X(double, probability)

#define STOCHASTIC_DISCOUNTED_GRAPH_FIELDS(X) /* none */

// --- One macro to generate everything ---
// Only this macro invocation is needed per game!
DEFINE_GAME_GRAPH(Stochastic_Discounted, STOCHASTIC_DISCOUNTED_VERTEX_FIELDS, STOCHASTIC_DISCOUNTED_EDGE_FIELDS, STOCHASTIC_DISCOUNTED_GRAPH_FIELDS)

#undef STOCHASTIC_DISCOUNTED_VERTEX_FIELDS
#undef STOCHASTIC_DISCOUNTED_EDGE_FIELDS
#undef STOCHASTIC_DISCOUNTED_GRAPH_FIELDS

// Additional utility functions specific to discounted games

/**
 * @brief Find vertex by name in a discounted graph
 * @param graph The discounted graph
 * @param name Vertex name
 * @return Vertex descriptor or boost::graph_traits<DiscountedGraph>::null_vertex() if not found
 */

struct VertexFilter {
    const Stochastic_DiscountedGraph *g;
    VertexFilter() = default;
    VertexFilter(const Stochastic_DiscountedGraph &graph) : g(&graph) {}

    bool operator()(Stochastic_DiscountedGraph::vertex_descriptor v) const {
        return (*g)[v].player == 1;
    }
};

struct CycleDetector : public boost::dfs_visitor<> {
    bool has_cycle = false;

    template <typename Edge, typename Graph>
    void back_edge(Edge, const Graph &) {
        has_cycle = true;
    }
};

inline Stochastic_DiscountedVertex find_vertex(const Stochastic_DiscountedGraph &graph, const std::string &name) {
    const auto [vertices_begin, vertices_end] = boost::vertices(graph);
    const auto found = std::find_if(vertices_begin, vertices_end, [&graph, &name](const auto &vertex) {
        return graph[vertex].name == name;
    });
    return (found != vertices_end) ? *found : boost::graph_traits<Stochastic_DiscountedGraph>::null_vertex();
}

/**
 * @brief Checks if the given Stochastic_DiscountedGraph is valid.
 *
 * This function verifies the following conditions:
 *   - Each vertex has a valid player value (-1, 0 or 1).
 *   - Each vertex has at least one outgoing edge.
 *   - Each edge having source vertex of player 0 or 1 has weight and a discount factor in the range (0.0, 1.0).
 *   - Each edge having source vertex of player -1 has a probability in the range (0.0, 1.0].
 *   - The sum of the probability of the edges having source vertex of player -1 sum up to 1.
 *   - TODO There are no loops among vertices of player -1.
 *
 * @param graph The Stochastic_DiscountedGraph to validate.
 * @return true if the graph is valid, false otherwise.
 */
inline bool is_valid(const Stochastic_DiscountedGraph &graph) {
    const auto [vertices_begin, vertices_end] = boost::vertices(graph);

    // Check vertex validity
    for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
        // Check if player is valid (0 or 1)
        if (graph[vertex].player != -1 && graph[vertex].player != 0 && graph[vertex].player != 1) {
            return false;
        }
        // Check if vertex has at least one outgoing edge
        if (boost::out_degree(vertex, graph) == 0) {
            return false;
        }
    }

    // Check edge validity
    const auto [edges_begin, edges_end] = boost::edges(graph);
    for (const auto &edge : boost::make_iterator_range(edges_begin, edges_end)) {
        // Check if discount factor is valid
        auto s = boost::source(edge, graph);
        if (graph[s].player != -1) {
            const auto discount = graph[edge].discount;
            if ((discount <= 0.0) || (discount >= 1.0)) {
                return false;
            }
        }
    }

    // Check probability validity
    for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
        if (graph[vertex].player == -1) {
            double sum = 0.0;
            for (const auto &edge : boost::make_iterator_range(boost::out_edges(vertex, graph))) {
                const auto probability = graph[edge].probability;
                if ((probability <= 0.0) || (probability > 1.0)) {
                    return false;
                }
                sum += probability;
            }
            // Validate that the total sum of probabilities is 1.0 (with tolerance for floating point errors)
            if (std::abs(sum - 1.0) > 1e-8) {
                return false;
            }
        }
    }

    // Check loop probability
    auto filtered_graph = boost::make_filtered_graph(graph, boost::keep_all{}, VertexFilter(graph));
    CycleDetector vis;
    boost::depth_first_search(filtered_graph, boost::visitor(vis));
    if (vis.has_cycle) {
        return false;
    }

    return true;
}

/**
 * @brief Check for duplicate edges in the stochastic discounted graph.
 *
 * This function checks if there are multiple edges between the same pair of vertices,
 * regardless of edge properties like probability, weight, or discount. Two edges with
 * the same source and target vertices are considered duplicates.
 *
 * @param graph The Stochastic_DiscountedGraph to check for duplicate edges.
 * @throws std::runtime_error if duplicate edges are found.
 */
inline void check_no_duplicate_edges(const Stochastic_DiscountedGraph &graph) {
    std::set<std::pair<Stochastic_DiscountedVertex, Stochastic_DiscountedVertex>> seen_edges;
    const auto [edges_begin, edges_end] = boost::edges(graph);
    for (const auto &edge : boost::make_iterator_range(edges_begin, edges_end)) {
        auto source = boost::source(edge, graph);
        auto target = boost::target(edge, graph);
        const auto edge_key = std::make_pair(source, target);
        if (!seen_edges.insert(edge_key).second) {
            std::string source_name = graph[source].name;
            std::string target_name = graph[target].name;
            throw std::runtime_error("Duplicate edge found between vertices '" +
                                     source_name + "' and '" + target_name + "'");
        }
    }
}

/**
 * @brief Get minimum discount factor in the graph
 * @param graph The stochastic discounted graph
 * @return Minimum discount factor
 */
inline double get_min_discount(const Stochastic_DiscountedGraph &graph) {
    if (boost::num_edges(graph) == 0) {
        return 1.0;
    }

    const auto [edges_begin, edges_end] = boost::edges(graph);
    const auto min_edge = std::min_element(edges_begin, edges_end,
                                           [&graph](const auto &a, const auto &b) {
                                               return graph[a].discount < graph[b].discount;
                                           });
    return graph[*min_edge].discount;
}

/**
 * @brief Get maximum discount factor in the graph
 * @param graph The stochastic discounted graph
 * @return Maximum discount factor
 */
inline double get_max_discount(const Stochastic_DiscountedGraph &graph) {
    if (boost::num_edges(graph) == 0) {
        return 0.0;
    }

    const auto [edges_begin, edges_end] = boost::edges(graph);
    const auto max_edge = std::max_element(edges_begin, edges_end,
                                           [&graph](const auto &a, const auto &b) {
                                               return graph[a].discount < graph[b].discount;
                                           });
    return graph[*max_edge].discount;
}

/**
 * @brief Get edge weight distribution statistics
 * @param graph The stochastic discounted graph
 * @return Map from weight to count of edges
 */
inline std::map<double, int> get_weight_distribution(const Stochastic_DiscountedGraph &graph) {
    std::map<double, int> distribution;
    const auto [edges_begin, edges_end] = boost::edges(graph);
    std::for_each(edges_begin, edges_end, [&graph, &distribution](const auto &edge) {
        distribution[graph[edge].weight]++;
    });
    return distribution;
}

/**
 * @brief Iterator adapter for non-probabilistic vertices (player 0 and 1)
 * @param graph The stochastic discounted graph
 * @return Iterator range over vertices with player 0 or 1
 */
inline auto get_non_probabilistic_vertices(const Stochastic_DiscountedGraph &graph) {
    const auto [vertices_begin, vertices_end] = boost::vertices(graph);
    return boost::make_iterator_range(vertices_begin, vertices_end) |
           boost::adaptors::filtered([&graph](const auto &vertex) {
               return graph[vertex].player != -1;
           });
}

/**
 * @brief Get reachable non-probabilistic vertices through probabilistic edges
 * @param graph The stochastic discounted graph
 * @param source Non-probabilistic source vertex
 * @return Map from reachable non-probabilistic vertex to total probability of reaching it
 */
inline std::map<Stochastic_DiscountedVertex, double>
get_reachable_through_probabilistic(const Stochastic_DiscountedGraph &graph,
                                    Stochastic_DiscountedVertex source,
                                    Stochastic_DiscountedVertex successor) {
    std::map<Stochastic_DiscountedVertex, double> reachable;

    // If source is not non-probabilistic, return empty map
    if (graph[source].player == -1) {
        return reachable;
    }

    // BFS-like traversal through probabilistic vertices
    std::queue<std::pair<Stochastic_DiscountedVertex, double>> queue;
    std::set<Stochastic_DiscountedVertex> visited;

    // Start from the successor of source
    if (graph[successor].player == -1) {
        // Target is probabilistic, start with probability 1.0
        queue.push({successor, 1.0});
    } else {
        // Target is non-probabilistic, add directly
        reachable[successor] = 1.0;
    }

    // Process probabilistic vertices
    while (!queue.empty()) {
        auto [current, prob] = queue.front();
        queue.pop();

        if (visited.count(current)) {
            continue;
        }
        visited.insert(current);

        // Explore successors of current probabilistic vertex
        const auto [curr_out_begin, curr_out_end] = boost::out_edges(current, graph);
        for (auto edge_it = curr_out_begin; edge_it != curr_out_end; ++edge_it) {
            auto successor = boost::target(*edge_it, graph);
            double edge_prob = graph[*edge_it].probability;
            double total_prob = prob * edge_prob;

            if (graph[successor].player == -1) {
                // Successor is probabilistic, continue traversal
                if (!visited.count(successor)) {
                    queue.push({successor, total_prob});
                }
            } else {
                // Successor is non-probabilistic, accumulate probability
                reachable[successor] += total_prob;
            }
        }
    }

    return reachable;
}

} // namespace graphs
} // namespace ggg
