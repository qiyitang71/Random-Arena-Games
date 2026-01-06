#include "libggg/libggg.hpp"
#include "discounted_value_solver.hpp"

using namespace ggg;

// Use the unified macro to create a main function for the discounted value iteration solver
GGG_GAME_SOLVER_MAIN(graphs::DiscountedGraph, graphs::parse_Discounted_graph, solvers::DiscountedValueSolver)