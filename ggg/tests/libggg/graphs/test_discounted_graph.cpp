#include "libggg/graphs/discounted_graph.hpp"
#include <sstream>
#include <string>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(DiscountedGameGraphTests)

BOOST_AUTO_TEST_CASE(CreateSimpleDiscountedGame) {
    using namespace ggg::graphs;

    DiscountedGraph graph;

    // Add vertices
    auto v0 = add_vertex(graph, "start", 0);
    auto v1 = add_vertex(graph, "end", 1);

    // Add edge with weight and discount
    auto [edge, success] = add_edge(graph, v0, v1, "move", 10.0, 0.9);

    BOOST_TEST(success);
    BOOST_TEST(graph[v0].name == "start");
    BOOST_TEST(graph[v0].player == 0);
    BOOST_TEST(graph[v1].name == "end");
    BOOST_TEST(graph[v1].player == 1);
    BOOST_TEST(graph[edge].weight == 10.0);
    BOOST_TEST(graph[edge].discount == 0.9);
    BOOST_TEST(graph[edge].label == "move");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(DiscountedGraphTests)

BOOST_AUTO_TEST_CASE(ParseSimpleGraph) {
    using namespace ggg::graphs;

    std::string dot_content = R"(
digraph DiscountedGame {
  v0 [name="start", player=0];
  v1 [name="end", player=1];
  v0 -> v1 [weight=5.0, discount=0.8];
  v1 -> v0 [weight=2.0, discount=0.7];
}
)";

    std::istringstream input(dot_content);
    auto graph = parse_Discounted_graph(input);

    BOOST_TEST(graph != nullptr);
    BOOST_TEST(boost::num_vertices(*graph) == 2);
    BOOST_TEST(boost::num_edges(*graph) == 2);

    // Find vertices
    auto start = find_vertex(*graph, "start");
    auto end = find_vertex(*graph, "end");

    BOOST_TEST(start != boost::graph_traits<DiscountedGraph>::null_vertex());
    BOOST_TEST(end != boost::graph_traits<DiscountedGraph>::null_vertex());

    BOOST_TEST((*graph)[start].player == 0);
    BOOST_TEST((*graph)[end].player == 1);

    // Check edge properties
    const auto [edges_begin, edges_end] = boost::edges(*graph);
    BOOST_TEST(boost::num_edges(*graph) == 2); // Instead of comparing iterators

    // Find the edge from start to end
    bool found_edge = false;
    for (const auto &edge : boost::make_iterator_range(edges_begin, edges_end)) {
        auto source = boost::source(edge, *graph);
        auto target = boost::target(edge, *graph);
        if (source == start && target == end) {
            BOOST_TEST((*graph)[edge].weight == 5.0);
            BOOST_TEST((*graph)[edge].discount == 0.8);
            found_edge = true;
            break;
        }
    }
    BOOST_TEST(found_edge);
}

BOOST_AUTO_TEST_CASE(WriteGraphTest) {
    using namespace ggg::graphs;

    DiscountedGraph graph;
    auto v0 = add_vertex(graph, "test", 0);
    auto v1 = add_vertex(graph, "end", 1);
    add_edge(graph, v0, v1, "action", 3.5, 0.7);

    std::ostringstream output;
    bool success = write_Discounted_graph(graph, output);

    BOOST_TEST(success);
    std::string result = output.str();
    BOOST_TEST(result.find("weight=3.5") != std::string::npos);
    BOOST_TEST(result.find("discount=0.7") != std::string::npos);
    BOOST_TEST(result.find("player=0") != std::string::npos);
    BOOST_TEST(result.find("player=1") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()
