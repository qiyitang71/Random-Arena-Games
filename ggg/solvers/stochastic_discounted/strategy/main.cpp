#include "libggg/libggg.hpp"
#include "stochastic_discounted_strategy_solver.hpp"

using namespace ggg;

// Use the unified macro to create a main function for the discounted strategy improvement solver
GGG_GAME_SOLVER_MAIN(graphs::Stochastic_DiscountedGraph, graphs::parse_Stochastic_Discounted_graph, solvers::StochasticDiscountedStrategySolver)