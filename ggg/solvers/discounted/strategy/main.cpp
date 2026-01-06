#include "libggg/libggg.hpp"
#include "discounted_strategy_solver.hpp"

using namespace ggg;

// Use the unified macro to create a main function for the discounted strategy improvement solver
GGG_GAME_SOLVER_MAIN(graphs::DiscountedGraph, graphs::parse_Discounted_graph, solvers::DiscountedStrategySolver)