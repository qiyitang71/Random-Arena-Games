#include "libggg/libggg.hpp"
#include "priority_promotion_solver.hpp"

using namespace ggg;

// Use the unified macro to create a main function for the priority promotion parity solver
GGG_GAME_SOLVER_MAIN(graphs::ParityGraph, graphs::parse_Parity_graph, solvers::PriorityPromotionSolver)
