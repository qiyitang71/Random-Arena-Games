#include "libggg/graphs/parity_graph.hpp"
#include <boost/graph/adjacency_list.hpp>

#define BOOST_TEST_MODULE Reachability Solver Test Suite
#include <boost/test/unit_test.hpp>

#include "reachability_solver.hpp"

using namespace ggg;
using namespace ggg::graphs;

BOOST_AUTO_TEST_SUITE(ReachabilityGameTests)

BOOST_AUTO_TEST_CASE(TestEmptyGame) {
    solvers::ReachabilitySolver solver;
    ParityGraph graph;
    auto solution = solver.solve(graph);

    BOOST_CHECK(solution.is_solved());
}

BOOST_AUTO_TEST_CASE(TestSingleTargetVertex) {
    solvers::ReachabilitySolver solver;
    ParityGraph graph;
    auto v0 = add_vertex(graph, "target", 0, 1); // player=0, priority=1 (target)

    auto solution = solver.solve(graph);

    BOOST_CHECK(solution.is_solved());
    BOOST_CHECK_EQUAL(solution.get_winning_player(v0), 0); // Player 0 wins target vertex
}

BOOST_AUTO_TEST_CASE(TestSimpleReachabilityGame) {
    solvers::ReachabilitySolver solver;
    ParityGraph graph;

    // Create vertices using the helper function
    auto v0 = add_vertex(graph, "start", 0, 0);  // player=0, priority=0
    auto v1 = add_vertex(graph, "choice", 1, 0); // player=1, priority=0
    auto v2 = add_vertex(graph, "target", 0, 1); // player=0, priority=1 (target)
    auto v3 = add_vertex(graph, "trap", 0, 0);   // player=0, priority=0

    // Add edges
    boost::add_edge(v0, v1, graph);
    boost::add_edge(v1, v2, graph);
    boost::add_edge(v1, v3, graph);
    boost::add_edge(v3, v3, graph); // self-loop

    auto solution = solver.solve(graph);

    BOOST_CHECK(solution.is_solved());

    // In this game, Player 1 controls the choice vertex and can prevent reaching target
    // So Player 1 should win from start and choice, Player 0 wins the target itself
    BOOST_CHECK_EQUAL(solution.get_winning_player(v0), 1); // start won by Player 1
    BOOST_CHECK_EQUAL(solution.get_winning_player(v1), 1); // choice won by Player 1
    BOOST_CHECK_EQUAL(solution.get_winning_player(v2), 0); // target won by Player 0
    BOOST_CHECK_EQUAL(solution.get_winning_player(v3), 1); // trap won by Player 1
}

BOOST_AUTO_TEST_CASE(TestPlayer0CanWin) {
    solvers::ReachabilitySolver solver;
    ParityGraph graph;

    // Create vertices where Player 0 can force reaching target
    auto v0 = add_vertex(graph, "start", 0, 0);   // player=0, priority=0
    auto v1 = add_vertex(graph, "control", 0, 0); // player=0, priority=0
    auto v2 = add_vertex(graph, "target", 0, 1);  // player=0, priority=1 (target)

    // Add edges
    boost::add_edge(v0, v1, graph);
    boost::add_edge(v1, v2, graph);

    auto solution = solver.solve(graph);

    BOOST_CHECK(solution.is_solved());

    // Player 0 controls the entire path, so should win all vertices
    BOOST_CHECK_EQUAL(solution.get_winning_player(v0), 0);
    BOOST_CHECK_EQUAL(solution.get_winning_player(v1), 0);
    BOOST_CHECK_EQUAL(solution.get_winning_player(v2), 0);
}

BOOST_AUTO_TEST_CASE(TestInvalidGame) {
    solvers::ReachabilitySolver solver;
    ParityGraph graph;
    auto v0 = add_vertex(graph, "invalid", 0, 2); // invalid priority (not 0 or 1)

    auto solution = solver.solve(graph);

    BOOST_CHECK(solution.is_solved()); // Should still be marked as solved even with invalid input
}

BOOST_AUTO_TEST_CASE(TestNoTargetVertices) {
    solvers::ReachabilitySolver solver;
    ParityGraph graph;

    // Create graph with no target vertices (all priority 0)
    auto v0 = add_vertex(graph, "v0", 0, 0); // player=0, priority=0
    auto v1 = add_vertex(graph, "v1", 1, 0); // player=1, priority=0

    boost::add_edge(v0, v1, graph);

    auto solution = solver.solve(graph);

    BOOST_CHECK(solution.is_solved());

    // With no targets, Player 1 should win everywhere
    BOOST_CHECK_EQUAL(solution.get_winning_player(v0), 1);
    BOOST_CHECK_EQUAL(solution.get_winning_player(v1), 1);
}

BOOST_AUTO_TEST_CASE(TestSolverName) {
    solvers::ReachabilitySolver solver;
    std::string name = solver.get_name();
    BOOST_CHECK(!name.empty());
    BOOST_CHECK(name.find("Reachability") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()
