#include "libggg/libggg.hpp"
#include "mse_solver.hpp"

using namespace ggg;

// Use the unified macro to create a main function for the MSE mean payoff solver
GGG_GAME_SOLVER_MAIN(graphs::MeanPayoffGraph, graphs::parse_MeanPayoff_graph, solvers::MSESolver)
