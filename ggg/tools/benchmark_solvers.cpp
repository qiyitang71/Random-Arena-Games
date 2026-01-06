#include "benchmark_solvers.hpp"
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include <stdio.h>
#ifdef _WIN32
#include <io.h>
#define popen _popen
#define pclose _pclose
#endif

namespace po = boost::program_options;
namespace fs = boost::filesystem;

/**
 * @brief Tool to run all solvers on a set of game files and compare performance
 *
 * This tool finds all solvers for a given game type, runs them on all
 * game files in a directory, and outputs timing comparisons.
 */
class SolverBenchmark {
  public:
    struct BenchmarkResult {
        std::string solver_name;
        std::string game_file;
        double solve_time;
        bool success;
        std::string error_message;
    };

    /**
     * @brief Main function for the benchmark tool
     */
    static int run(int argc, char *argv[]) {
        try {
            po::options_description desc("Solver Benchmark Options");
            desc.add_options()("help,h", "Show help message");
            desc.add_options()("game-type,g", po::value<std::string>()->required(), "Game type (parity, meanpayoff)");
            desc.add_options()("solver-path,p", po::value<std::string>()->default_value("./solvers"), "Path to solver binaries");
            desc.add_options()("games-dir,d", po::value<std::string>()->required(), "Directory containing game files in DOT format");
            desc.add_options()("csv", "Output results in CSV format");
            desc.add_options()("timeout,t", po::value<int>()->default_value(30), "Timeout in seconds for each solver run");
            desc.add_options()("verbose,v", "Show detailed output");

            po::variables_map vm;
            po::store(po::parse_command_line(argc, argv, desc), vm);

            if (vm.count("help")) {
                std::cout << desc << std::endl;
                return 0;
            }

            po::notify(vm);

            std::string game_type = vm["game-type"].as<std::string>();
            std::string solver_path = vm["solver-path"].as<std::string>();
            std::string games_dir = vm["games-dir"].as<std::string>();
            bool csv_output = vm.count("csv") > 0;
            int timeout = vm["timeout"].as<int>();
            bool verbose = vm.count("verbose") > 0;

            return run_benchmark(game_type, solver_path, games_dir, csv_output, timeout, verbose);

        } catch (const std::exception &e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
    }

  private:
    /**
     * @brief Run benchmark comparing all solvers
     */
    static int run_benchmark(const std::string &game_type,
                             const std::string &solver_path,
                             const std::string &games_dir,
                             bool csv_output,
                             int timeout,
                             bool verbose) {

        // Find all solvers
        std::vector<std::string> solvers = find_solvers(game_type, solver_path);
        if (solvers.empty()) {
            std::cerr << "No solvers found for game type '" << game_type << "'" << std::endl;
            return 1;
        }

        // Find all game files
        std::vector<std::string> game_files = find_game_files(games_dir);
        if (game_files.empty()) {
            std::cerr << "No game files found in directory '" << games_dir << "'" << std::endl;
            return 1;
        }

        if (verbose && !csv_output) {
            std::cout << "Found " << solvers.size() << " solvers and "
                      << game_files.size() << " game files" << std::endl;
            std::cout << "Running benchmark..." << std::endl
                      << std::endl;
        }

        std::vector<BenchmarkResult> results;

        // Run each solver on each game file
        for (const auto &solver : solvers) {
            for (const auto &game_file : game_files) {
                BenchmarkResult result = run_solver(solver, game_file, timeout, verbose && !csv_output);
                results.push_back(result);
            }
        }

        // Output results
        if (csv_output) {
            output_csv(results);
        } else {
            output_table(results, game_files, solvers);
        }

        return 0;
    }

    /**
     * @brief Find all solvers for a game type
     */
    static std::vector<std::string> find_solvers(const std::string &game_type,
                                                 const std::string &solver_path) {
        std::vector<std::string> solvers;
        fs::path game_type_dir = fs::path(solver_path) / game_type;

        if (!fs::exists(game_type_dir) || !fs::is_directory(game_type_dir)) {
            return solvers;
        }

        try {
            for (fs::recursive_directory_iterator it(game_type_dir);
                 it != fs::recursive_directory_iterator(); ++it) {

                if (fs::is_regular_file(it->status()) &&
                    is_executable(it->path())) {
                    solvers.push_back(it->path().string());
                }
            }
        } catch (const fs::filesystem_error &) {
            // Ignore errors
        }

        std::sort(solvers.begin(), solvers.end());
        return solvers;
    }

    /**
     * @brief Find all DOT game files in directory
     */
    static std::vector<std::string> find_game_files(const std::string &games_dir) {
        std::vector<std::string> game_files;

        if (!fs::exists(games_dir) || !fs::is_directory(games_dir)) {
            return game_files;
        }

        try {
            for (fs::directory_iterator it(games_dir);
                 it != fs::directory_iterator(); ++it) {

                if (fs::is_regular_file(it->status()) &&
                    it->path().extension() == ".dot") {
                    game_files.push_back(it->path().string());
                }
            }
        } catch (const fs::filesystem_error &) {
            // Ignore errors
        }

        std::sort(game_files.begin(), game_files.end());
        return game_files;
    }

    /**
     * @brief Run a solver on a game file
     */
    static BenchmarkResult run_solver(const std::string &solver,
                                      const std::string &game_file,
                                      int timeout,
                                      bool verbose) {
        BenchmarkResult result;
        result.solver_name = fs::path(solver).stem().string();
        result.game_file = fs::path(game_file).filename().string();

        std::string command = solver + " -i " + game_file + " --time-only --csv 2>/dev/null";

        if (verbose) {
            std::cout << "Running " << result.solver_name << " on " << result.game_file << "... ";
        }

        auto start = std::chrono::high_resolution_clock::now();

        FILE *pipe = popen(command.c_str(), "r");
        if (!pipe) {
            result.success = false;
            result.error_message = "Failed to execute solver";
            if (verbose)
                std::cout << "FAILED (execution error)" << std::endl;
            return result;
        }

        char buffer[256];
        std::string output = "";
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            output += buffer;
        }
        int exit_code = pclose(pipe);

        auto end = std::chrono::high_resolution_clock::now();
        auto wall_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double wall_time_seconds = wall_time.count() / 1000000.0;

        if (exit_code == 0 && !output.empty()) {
            try {
                result.solve_time = std::stod(output);
                result.success = true;
                if (verbose) {
                    std::cout << "OK (" << std::fixed << std::setprecision(6)
                              << result.solve_time << "s)" << std::endl;
                }
            } catch (...) {
                result.success = false;
                result.error_message = "Invalid output format";
                if (verbose)
                    std::cout << "FAILED (invalid output)" << std::endl;
            }
        } else {
            result.success = false;
            result.error_message = "Solver failed or timed out";
            if (verbose)
                std::cout << "FAILED (exit code " << exit_code << ")" << std::endl;
        }

        return result;
    }

    /**
     * @brief Check if a file is executable
     */
    static bool is_executable(const fs::path &path) {
        try {
            fs::file_status status = fs::status(path);
            return (status.permissions() & fs::owner_exe) != fs::no_perms;
        } catch (...) {
            return false;
        }
    }

    /**
     * @brief Output results in CSV format
     */
    static void output_csv(const std::vector<BenchmarkResult> &results) {
        std::cout << "solver,game_file,solve_time,success,error_message" << std::endl;

        for (const auto &result : results) {
            std::cout << result.solver_name << ","
                      << result.game_file << ",";

            if (result.success) {
                std::cout << std::fixed << std::setprecision(6) << result.solve_time;
            } else {
                std::cout << "N/A";
            }

            std::cout << "," << (result.success ? "true" : "false") << ","
                      << result.error_message << std::endl;
        }
    }

    /**
     * @brief Output results in table format
     */
    static void output_table(const std::vector<BenchmarkResult> &results,
                             const std::vector<std::string> &game_files,
                             const std::vector<std::string> &solvers) {

        // Create table
        std::cout << std::setw(15) << "Game \\ Solver";
        for (const auto &solver : solvers) {
            std::string solver_name = fs::path(solver).stem().string();
            std::cout << std::setw(15) << solver_name;
        }
        std::cout << std::endl;

        std::cout << std::string(15 + solvers.size() * 15, '-') << std::endl;

        for (const auto &game_file : game_files) {
            std::string game_name = fs::path(game_file).filename().string();
            std::cout << std::setw(15) << game_name;

            for (const auto &solver : solvers) {
                std::string solver_name = fs::path(solver).stem().string();

                // Find result for this solver/game combination
                auto it = std::find_if(results.begin(), results.end(),
                                       [&](const BenchmarkResult &r) {
                                           return r.solver_name == solver_name &&
                                                  r.game_file == game_name;
                                       });

                if (it != results.end() && it->success) {
                    std::cout << std::setw(15) << std::fixed << std::setprecision(6)
                              << it->solve_time << "s";
                } else {
                    std::cout << std::setw(15) << "FAILED";
                }
            }
            std::cout << std::endl;
        }
    }
};

namespace ggg_tools {
int run_benchmark_solvers(int argc, char *argv[]) {
    return SolverBenchmark::run(argc, argv);
}
} // namespace ggg_tools