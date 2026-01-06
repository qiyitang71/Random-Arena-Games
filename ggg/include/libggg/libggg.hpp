#pragma once

/**
 * @file libggg.hpp
 * @brief Main header file for the Game Graph Gym library
 *
 * This header includes all the main components of the Game Graph Gym library,
 * providing a convenient single include for users of the library.
 */

// Game types
#include "libggg/graphs/discounted_graph.hpp"
#include "libggg/graphs/mean_payoff_graph.hpp"
#include "libggg/graphs/parity_graph.hpp"
#include "libggg/graphs/stochastic_discounted_graph.hpp"

// Parsers
// Parser functions are now included directly in graph headers

// Solvers
#include "libggg/solvers/solver.hpp"

// Utilities
#include "libggg/utils/solver_wrapper.hpp"

/**
 * @namespace ggg
 * @brief Main namespace for the Game Graph Gym library
 *
 * This namespace contains all classes and functions provided by the
 * Game Graph Gym library for working with games on graphs.
 */
namespace ggg {

/**
 * @brief Library version information
 */
constexpr int VERSION_MAJOR = 1;
constexpr int VERSION_MINOR = 0;
constexpr int VERSION_PATCH = 0;
constexpr const char *VERSION_STRING = "1.0.0";

/**
 * @brief Get the library version as a string
 * @return Version string in format "major.minor.patch"
 */
inline const char *get_version() {
    return VERSION_STRING;
}

} // namespace ggg
