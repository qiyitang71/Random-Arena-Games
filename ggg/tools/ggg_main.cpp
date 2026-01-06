#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <string>
#include <vector>

// Include libggg headers
#include "libggg/libggg.hpp"
#include "libggg/utils/logging.hpp"

// Include tool headers
#include "benchmark_solvers.hpp"
#include "generate_games.hpp"
#include "list_solvers.hpp"

namespace po = boost::program_options;
namespace fs = boost::filesystem;

/**
 * @brief Show help message with available subcommands
 */
void show_help() {
    std::cout << "Game Graph Gym (GGG) - A framework for game-theoretic solvers\n\n";
    std::cout << "Usage: ggg [options] <subcommand> [subcommand-options]\n\n";
    std::cout << "Global Options:\n";
    std::cout << "  -h, --help                Show this help message\n";
    std::cout << "  -V, --version             Show version information\n";
    std::cout << "  -v                        Increase verbosity (can be used multiple times)\n";
    std::cout << "  -q                        Decrease verbosity (quiet mode)\n\n";
    std::cout << "Available subcommands:\n";
    std::cout << "  benchmark     Run benchmark tests on solvers\n";
    std::cout << "  generate      Generate random game graphs\n";
    std::cout << "  list-solvers  List available solvers for a game type\n";
    std::cout << "\nUse 'ggg <subcommand> --help' for help on a specific subcommand.\n";
}

/**
 * @brief Show version information
 */
void show_version() {
    std::cout << "Game Graph Gym (GGG) version " << ggg::get_version() << std::endl;
}

/**
 * @brief Handle benchmark subcommand
 */
int handle_benchmark(const std::vector<std::string> &args) {
    // Convert back to argc/argv format
    std::vector<char *> c_args;
    c_args.push_back(const_cast<char *>("ggg benchmark"));
    for (const auto &arg : args) {
        c_args.push_back(const_cast<char *>(arg.c_str()));
    }

    return ggg_tools::run_benchmark_solvers(c_args.size(), c_args.data());
}

/**
 * @brief Handle list-solvers subcommand
 */
int handle_list_solvers(const std::vector<std::string> &args) {
    // Convert back to argc/argv format
    std::vector<char *> c_args;
    c_args.push_back(const_cast<char *>("ggg list-solvers"));
    for (const auto &arg : args) {
        c_args.push_back(const_cast<char *>(arg.c_str()));
    }

    return ggg_tools::run_list_solvers(c_args.size(), c_args.data());
}

/**
 * @brief Handle generate subcommand
 */
int handle_generate(const std::vector<std::string> &args) {
    // Convert back to argc/argv format
    std::vector<char *> c_args;
    c_args.push_back(const_cast<char *>("ggg generate"));
    for (const auto &arg : args) {
        c_args.push_back(const_cast<char *>(arg.c_str()));
    }

    return ggg_tools::run_generate_games(c_args.size(), c_args.data());
}

int main(int argc, char *argv[]) {
    try {
        // Convert to vector for easier handling
        std::vector<std::string> args(argv, argv + argc);

        if (args.size() < 2) {
            show_help();
            return 1;
        }

        // Process global options
        int verbosity = 0;
        bool quiet = false;
        std::vector<std::string> remaining_args;

        for (size_t i = 1; i < args.size(); ++i) {
            if (args[i] == "-h" || args[i] == "--help") {
                show_help();
                return 0;
            } else if (args[i] == "-V" || args[i] == "--version") {
                show_version();
                return 0;
            } else if (args[i] == "-v") {
                verbosity++;
            } else if (args[i] == "-q") {
                quiet = true;
            } else {
                // This and all remaining args go to the subcommand
                remaining_args.assign(args.begin() + i, args.end());
                break;
            }
        }

        // Set log level based on verbosity
        if (quiet) {
            ggg::utils::set_log_level(ggg::utils::LogLevel::ERROR);
        } else if (verbosity > 0) {
            ggg::utils::set_log_level(ggg::utils::verbosity_to_log_level(verbosity));
        }

        if (remaining_args.empty()) {
            std::cerr << "Error: No subcommand specified.\n\n";
            show_help();
            return 1;
        }

        // Find and execute the subcommand
        std::string subcommand = remaining_args[0];
        std::vector<std::string> sub_args(remaining_args.begin() + 1, remaining_args.end());

        if (subcommand == "benchmark") {
            return handle_benchmark(sub_args);
        } else if (subcommand == "generate") {
            return handle_generate(sub_args);
        } else if (subcommand == "list-solvers") {
            return handle_list_solvers(sub_args);

        } else {
            std::cerr << "Error: Unknown subcommand '" << subcommand << "'\n\n";
            show_help();
            return 1;
        }

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
