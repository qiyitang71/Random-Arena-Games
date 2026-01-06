#pragma once

namespace ggg_tools {
/**
 * @brief Run the unified game generator
 *
 * This function parses common parameters and delegates to the appropriate
 * game-specific generator based on the specified type.
 *
 * @param argc Argument count
 * @param argv Argument values
 * @return Exit code (0 for success)
 */
int run_generate_games(int argc, char *argv[]);
} // namespace ggg_tools