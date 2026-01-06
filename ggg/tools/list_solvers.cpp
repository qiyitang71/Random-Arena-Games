#include "list_solvers.hpp"
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <algorithm>
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
 * @brief Tool to list all available solvers for a given game type
 *
 * This tool scans the SOLVER_PATH directory for binaries that implement
 * solvers for specific game types and lists them.
 */
class SolverLister {
  public:
    /**
     * @brief Main function for the solver lister tool
     */
    static int run(int argc, char *argv[]) {
        try {
            po::options_description desc("Solver Lister Options");
            desc.add_options()("help,h", "Show help message");
            desc.add_options()("game-type,g", po::value<std::string>()->required(), "Game type (parity, meanpayoff)");
            desc.add_options()("solver-path,p", po::value<std::string>()->default_value("./solvers"), "Path to solver binaries");
            desc.add_options()("verbose,v", "Show detailed information about each solver");

            po::variables_map vm;
            po::store(po::parse_command_line(argc, argv, desc), vm);

            if (vm.count("help")) {
                std::cout << desc << std::endl;
                return 0;
            }

            po::notify(vm);

            std::string game_type = vm["game-type"].as<std::string>();
            std::string solver_path = vm["solver-path"].as<std::string>();
            bool verbose = vm.count("verbose") > 0;

            return list_solvers(game_type, solver_path, verbose);

        } catch (const std::exception &e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
    }

  private:
    /**
     * @brief List all solvers for a given game type
     */
    static int list_solvers(const std::string &game_type,
                            const std::string &solver_path,
                            bool verbose) {

        // Validate game type
        if (game_type != "parity" && game_type != "meanpayoff") {
            std::cerr << "Error: Invalid game type '" << game_type
                      << "'. Valid types: parity, meanpayoff" << std::endl;
            return 1;
        }

        // Construct path to game type directory
        fs::path game_type_dir = fs::path(solver_path) / game_type;

        if (!fs::exists(game_type_dir) || !fs::is_directory(game_type_dir)) {
            std::cout << "No solvers found for game type '" << game_type << "'" << std::endl;
            return 0;
        }

        std::vector<std::string> solvers;

        // Find all executable files in subdirectories
        try {
            for (fs::recursive_directory_iterator it(game_type_dir);
                 it != fs::recursive_directory_iterator(); ++it) {

                if (fs::is_regular_file(it->status()) &&
                    is_executable(it->path())) {
                    solvers.push_back(it->path().string());
                }
            }
        } catch (const fs::filesystem_error &e) {
            std::cerr << "Error scanning directory: " << e.what() << std::endl;
            return 1;
        }

        if (solvers.empty()) {
            std::cout << "No solvers found for game type '" << game_type << "'" << std::endl;
            return 0;
        }

        // Sort solvers by name
        std::sort(solvers.begin(), solvers.end());

        std::cout << "Available solvers for '" << game_type << "' games:" << std::endl;
        std::cout << std::string(50, '-') << std::endl;

        for (const auto &solver : solvers) {
            fs::path solver_path(solver);
            std::string solver_name = solver_path.stem().string();

            if (verbose) {
                std::cout << "Name: " << solver_name << std::endl;
                std::cout << "Path: " << solver << std::endl;

                // Try to get solver description
                std::string description = get_solver_description(solver);
                if (!description.empty()) {
                    std::cout << "Description: " << description << std::endl;
                }
                std::cout << std::endl;
            } else {
                std::cout << "  " << solver_name << std::endl;
            }
        }

        return 0;
    }

    /**
     * @brief Check if a file is executable
     */
    static bool is_executable(const fs::path &path) {
        try {
            // Simple check for executable permission
            fs::file_status status = fs::status(path);
            return (status.permissions() & fs::owner_exe) != fs::no_perms;
        } catch (...) {
            return false;
        }
    }

    /**
     * @brief Get solver description by running it with --solver-name
     */
    static std::string get_solver_description(const std::string &solver_path) {
        try {
            std::string command = solver_path + " --solver-name 2>/dev/null";
            FILE *pipe = popen(command.c_str(), "r");
            if (!pipe)
                return "";

            char buffer[256];
            std::string result = "";
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                result += buffer;
            }
            pclose(pipe);

            // Remove trailing newline
            if (!result.empty() && result.back() == '\n') {
                result.pop_back();
            }

            return result;
        } catch (...) {
            return "";
        }
    }
};

namespace ggg_tools {
int run_list_solvers(int argc, char *argv[]) {
    return SolverLister::run(argc, argv);
}
} // namespace ggg_tools