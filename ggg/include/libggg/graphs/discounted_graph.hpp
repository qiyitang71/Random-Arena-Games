#pragma once
#include "libggg/graphs/graph_utilities.hpp"
#include <algorithm>
#include <map>

namespace ggg {
namespace graphs {

// --- Define property field lists as macros ---
#define DISCOUNTED_VERTEX_FIELDS(X) \
    X(std::string, name)            \
    X(int, player)

#define DISCOUNTED_EDGE_FIELDS(X) \
    X(std::string, label)         \
    X(double, weight)             \
    X(double, discount)

#define DISCOUNTED_GRAPH_FIELDS(X) /* none */

// --- One macro to generate everything ---
// Only this macro invocation is needed per game!
DEFINE_GAME_GRAPH(Discounted, DISCOUNTED_VERTEX_FIELDS, DISCOUNTED_EDGE_FIELDS, DISCOUNTED_GRAPH_FIELDS)

#undef DISCOUNTED_VERTEX_FIELDS
#undef DISCOUNTED_EDGE_FIELDS
#undef DISCOUNTED_GRAPH_FIELDS

// Additional utility functions specific to discounted games

/**
 * @brief Find vertex by name in a discounted graph
 * @param graph The discounted graph
 * @param name Vertex name
 * @return Vertex descriptor or boost::graph_traits<DiscountedGraph>::null_vertex() if not found
 */
inline DiscountedVertex find_vertex(const DiscountedGraph &graph, const std::string &name) {
    const auto [vertices_begin, vertices_end] = boost::vertices(graph);
    const auto found = std::find_if(vertices_begin, vertices_end, [&graph, &name](const auto &vertex) {
        return graph[vertex].name == name;
    });
    return (found != vertices_end) ? *found : boost::graph_traits<DiscountedGraph>::null_vertex();
}

/**
 * @brief Checks if the given DiscountedGraph is valid.
 *
 * This function verifies the following conditions:
 *   - Each vertex has a valid player value (0 or 1).
 *   - Each vertex has at least one outgoing edge.
 *   - Each edge has a discount factor in the range [0.0, 1.0).
 *
 * @param graph The DiscountedGraph to validate.
 * @return true if the graph is valid, false otherwise.
 */
inline bool is_valid(const DiscountedGraph &graph) {
    const auto [vertices_begin, vertices_end] = boost::vertices(graph);

    // Check vertex validity
    for (const auto &vertex : boost::make_iterator_range(vertices_begin, vertices_end)) {
        // Check if player is valid (0 or 1)
        if (graph[vertex].player != 0 && graph[vertex].player != 1) {
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
        const auto discount = graph[edge].discount;
        if ((discount < 0.0) || (discount >= 1.0)) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Get minimum discount factor in the graph
 * @param graph The discounted graph
 * @return Minimum discount factor
 */
inline double get_min_discount(const DiscountedGraph &graph) {
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
 * @param graph The discounted graph
 * @return Maximum discount factor
 */
inline double get_max_discount(const DiscountedGraph &graph) {
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
 * @param graph The discounted graph
 * @return Map from weight to count of edges
 */
inline std::map<double, int> get_weight_distribution(const DiscountedGraph &graph) {
    std::map<double, int> distribution;
    const auto [edges_begin, edges_end] = boost::edges(graph);
    std::for_each(edges_begin, edges_end, [&graph, &distribution](const auto &edge) {
        distribution[graph[edge].weight]++;
    });
    return distribution;
}

} // namespace graphs
} // namespace ggg
