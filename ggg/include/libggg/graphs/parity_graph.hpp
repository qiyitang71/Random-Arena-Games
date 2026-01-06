#pragma once
#include "libggg/graphs/graph_utilities.hpp"
#include <algorithm>
#include <stdexcept>

namespace ggg {
namespace graphs {

// --- Define property field lists as macros ---
#define PARITY_VERTEX_FIELDS(X) \
    X(std::string, name)        \
    X(int, player)              \
    X(int, priority)

#define PARITY_EDGE_FIELDS(X) \
    X(std::string, label)

#define PARITY_GRAPH_FIELDS(X) /* none */

// Define the graph type and parser for it
DEFINE_GAME_GRAPH(Parity, PARITY_VERTEX_FIELDS, PARITY_EDGE_FIELDS, PARITY_GRAPH_FIELDS)

#undef PARITY_VERTEX_FIELDS
#undef PARITY_EDGE_FIELDS
#undef PARITY_GRAPH_FIELDS

// Additional utility functions specific to parity games

/**
 * @brief Checks if the given ParityGraph is valid.
 *
 * A valid ParityGraph satisfies the following conditions for every vertex:
 * - The player value is either 0 or 1.
 * - The priority is non-negative.
 * - The vertex has at least one outgoing edge.
 *
 * @param graph The ParityGraph to validate.
 * @return True if the graph is valid, false otherwise.
 */
inline bool is_valid(const ParityGraph &graph) {
    const auto [vertices_begin, vertices_end] = boost::vertices(graph);
    return std::all_of(vertices_begin, vertices_end, [&graph](const auto &vertex) {
        if (graph[vertex].player != 0 && graph[vertex].player != 1) {
            return false;
        }
        if (graph[vertex].priority < 0) {
            return false;
        }
        return boost::out_degree(vertex, graph) > 0;
    });
}

/**
 * @brief Check for duplicate edges in the parity graph.
 *
 * This function checks if there are multiple edges between the same pair of vertices,
 * regardless of edge properties. Two edges with the same source and target vertices
 * are considered duplicates.
 *
 * @param graph The ParityGraph to check for duplicate edges.
 * @throws std::runtime_error if duplicate edges are found.
 */
inline void check_no_duplicate_edges(const ParityGraph &graph) {
    std::set<std::pair<ParityVertex, ParityVertex>> seen_edges;
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

} // namespace graphs
} // namespace ggg
