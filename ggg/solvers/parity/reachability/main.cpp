#include "libggg/libggg.hpp"
#include "reachability_solver.hpp"

using namespace ggg;

// Use the unified macro to create a main function for the Reachability game solver
GGG_GAME_SOLVER_MAIN(graphs::ParityGraph, graphs::parse_Parity_graph, solvers::ReachabilitySolver)
