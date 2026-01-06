#include "libggg/libggg.hpp"
#include "msca_solver.hpp"

using namespace ggg;

// Use the unified macro to create a main function for the MSCA mean payoff solver
GGG_GAME_SOLVER_MAIN(graphs::MeanPayoffGraph, graphs::parse_MeanPayoff_graph, solvers::MSCASolver)
