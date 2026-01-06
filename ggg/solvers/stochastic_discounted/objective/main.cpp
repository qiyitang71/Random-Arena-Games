#include "libggg/libggg.hpp"
#include "stochastic_discounted_objective_solver.hpp"

using namespace ggg;

// Use the unified macro to create a main function for the stochastic discounted objective improvement solver
GGG_GAME_SOLVER_MAIN(graphs::Stochastic_DiscountedGraph, graphs::parse_Stochastic_Discounted_graph, solvers::StochasticDiscountedObjectiveSolver)