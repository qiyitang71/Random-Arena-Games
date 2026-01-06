#include "libggg/libggg.hpp"
#include "buchi_solver.hpp"

using namespace ggg;

// Use the unified macro to create a main function for the Buchi game solver
GGG_GAME_SOLVER_MAIN(graphs::ParityGraph, graphs::parse_Parity_graph, solvers::BuchiSolver)
