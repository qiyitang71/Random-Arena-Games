#pragma once

#include "libggg/utils/logging.hpp"
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/property_map/dynamic_property_map.hpp>


namespace ggg {
namespace graphs {

// --- Helper macros for property struct field generation ---
#define PROPERTY_STRUCT_FIELD(type, name) type name;

// These macros work within the context of register_*_dynamic_properties functions
// where dp, g, and Props are in scope
#define REGISTER_VERTEX_FIELD_IMPL(type, name) dp.property(#name, boost::get(&Props::name, g));
#define REGISTER_EDGE_FIELD_IMPL(type, name) dp.property(#name, boost::get(&Props::name, g));
#define REGISTER_GRAPH_FIELD_IMPL(type, name) dp.property(#name, boost::get(&Props::name, g));

// Helper macros for generating add_vertex and add_edge parameters
#define ADD_VERTEX_PARAM(type, name) , const type &name
#define ADD_VERTEX_ASSIGN(type, name) v.name = name;
#define ADD_EDGE_PARAM(type, name) , const type &name
#define ADD_EDGE_ASSIGN(type, name) e.name = name;

// --- Main Macro: instantiate everything for a graph type ---
#define DEFINE_GAME_GRAPH(GAME_NAME, VERTEX_FIELDS, EDGE_FIELDS, GRAPH_FIELDS)                                                         \
    /* Property structs (internal linkage) */                                                                                          \
    namespace detail_##GAME_NAME {                                                                                                     \
        struct VertexProps {                                                                                                           \
            VERTEX_FIELDS(PROPERTY_STRUCT_FIELD)                                                                                       \
        };                                                                                                                             \
        struct EdgeProps {                                                                                                             \
            EDGE_FIELDS(PROPERTY_STRUCT_FIELD)                                                                                         \
        };                                                                                                                             \
        struct GraphProps {                                                                                                            \
            GRAPH_FIELDS(PROPERTY_STRUCT_FIELD)                                                                                        \
        };                                                                                                                             \
    }                                                                                                                                  \
    /* Graph type alias (public) */                                                                                                    \
    using GAME_NAME##Graph = boost::adjacency_list<                                                                                    \
        boost::setS, boost::vecS, boost::directedS,                                                                                    \
        detail_##GAME_NAME::VertexProps,                                                                                               \
        detail_##GAME_NAME::EdgeProps,                                                                                                 \
        detail_##GAME_NAME::GraphProps>;                                                                                               \
    /* Type aliases for convenience */                                                                                                 \
    using GAME_NAME##Vertex = boost::graph_traits<GAME_NAME##Graph>::vertex_descriptor;                                                \
    using GAME_NAME##Edge = boost::graph_traits<GAME_NAME##Graph>::edge_descriptor;                                                    \
    using GAME_NAME##VertexIterator = boost::graph_traits<GAME_NAME##Graph>::vertex_iterator;                                          \
    using GAME_NAME##EdgeIterator = boost::graph_traits<GAME_NAME##Graph>::edge_iterator;                                              \
    using GAME_NAME##OutEdgeIterator = boost::graph_traits<GAME_NAME##Graph>::out_edge_iterator;                                       \
    /* Dynamic property registration (internal utility) */                                                                             \
    inline void register_##GAME_NAME##_dynamic_properties(                                                                             \
        boost::dynamic_properties &dp, GAME_NAME##Graph &g) {                                                                          \
        /* Register vertex properties */                                                                                               \
        {                                                                                                                              \
            using Props [[maybe_unused]] = detail_##GAME_NAME::VertexProps;                                                            \
            VERTEX_FIELDS(REGISTER_VERTEX_FIELD_IMPL)                                                                                  \
        }                                                                                                                              \
        /* Register edge properties */                                                                                                 \
        {                                                                                                                              \
            using Props [[maybe_unused]] = detail_##GAME_NAME::EdgeProps;                                                              \
            EDGE_FIELDS(REGISTER_EDGE_FIELD_IMPL)                                                                                      \
        }                                                                                                                              \
        /* Register graph properties */                                                                                                \
        {                                                                                                                              \
            using Props [[maybe_unused]] = detail_##GAME_NAME::GraphProps;                                                             \
            GRAPH_FIELDS(REGISTER_GRAPH_FIELD_IMPL)                                                                                    \
        }                                                                                                                              \
    }                                                                                                                                  \
    /* Add vertex utility function */                                                                                                  \
    inline GAME_NAME##Vertex add_vertex(GAME_NAME##Graph &graph VERTEX_FIELDS(ADD_VERTEX_PARAM)) {                                     \
        GAME_NAME##Vertex vertex = boost::add_vertex(graph);                                                                           \
        auto &v = graph[vertex];                                                                                                       \
        VERTEX_FIELDS(ADD_VERTEX_ASSIGN)                                                                                               \
        return vertex;                                                                                                                 \
    }                                                                                                                                  \
    /* Add edge utility function */                                                                                                    \
    inline std::pair<GAME_NAME##Edge, bool> add_edge(GAME_NAME##Graph &graph,                                                          \
                                                     GAME_NAME##Vertex source, GAME_NAME##Vertex target EDGE_FIELDS(ADD_EDGE_PARAM)) { \
        auto result = boost::add_edge(source, target, graph);                                                                          \
        if (result.second) {                                                                                                           \
            auto &e = graph[result.first];                                                                                             \
            EDGE_FIELDS(ADD_EDGE_ASSIGN)                                                                                               \
        }                                                                                                                              \
        return result;                                                                                                                 \
    }                                                                                                                                  \
    /* Parser utilities (public) */                                                                                                    \
    inline std::shared_ptr<GAME_NAME##Graph> parse_##GAME_NAME##_graph(std::istream &in) {                                             \
        LGG_DEBUG("Starting DOT " #GAME_NAME " graph parsing from stream");                                                            \
        auto g = std::make_shared<GAME_NAME##Graph>();                                                                                 \
        boost::dynamic_properties dp(boost::ignore_other_properties);                                                                  \
        register_##GAME_NAME##_dynamic_properties(dp, *g);                                                                             \
        if (!boost::read_graphviz(in, *g, dp, "name")) {                                                                               \
            LGG_ERROR("Failed to parse DOT format");                                                                                   \
            return nullptr;                                                                                                            \
        }                                                                                                                              \
        return g;                                                                                                                      \
    }                                                                                                                                  \
    inline std::shared_ptr<GAME_NAME##Graph> parse_##GAME_NAME##_graph(const std::string &fn) {                                        \
        LGG_DEBUG("Parsing DOT file: ", fn);                                                                                           \
        std::ifstream file(fn);                                                                                                        \
        if (!file.is_open()) {                                                                                                         \
            LGG_ERROR("Failed to open file: ", fn);                                                                                    \
            return nullptr;                                                                                                            \
        }                                                                                                                              \
        return parse_##GAME_NAME##_graph(file);                                                                                        \
    }                                                                                                                                  \
    /* Writer utilities (public) */                                                                                                    \
    inline bool write_##GAME_NAME##_graph(const GAME_NAME##Graph &g, std::ostream &os) {                                               \
        boost::dynamic_properties dp(boost::ignore_other_properties);                                                                  \
        register_##GAME_NAME##_dynamic_properties(dp, const_cast<GAME_NAME##Graph &>(g));                                              \
        boost::write_graphviz_dp(os, g, dp, "name");                                                                                   \
        return true;                                                                                                                   \
    }                                                                                                                                  \
    inline bool write_##GAME_NAME##_graph(const GAME_NAME##Graph &g, const std::string &fn) {                                          \
        std::ofstream file(fn);                                                                                                        \
        if (!file.is_open())                                                                                                           \
            return false;                                                                                                              \
        return write_##GAME_NAME##_graph(g, file);                                                                                     \
    }

} // namespace graphs
} // namespace ggg
