#include "libggg/libggg.hpp"
#include "progressive_small_progress_measures_solver.hpp"

using namespace ggg;

// Use the new unified macro to create a main function for the PSPM parity solver
GGG_GAME_SOLVER_MAIN(graphs::ParityGraph, graphs::parse_Parity_graph, solvers::ProgressiveSmallProgressMeasuresSolver)
