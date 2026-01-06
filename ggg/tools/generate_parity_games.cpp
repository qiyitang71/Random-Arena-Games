#include "generate_parity_games.hpp"
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

/**
 * @brief Tool to generate random parity games for testing solvers
 *
 * This tool generates random parity games in DOT format and saves them
 * to a specified directory.
 */
class ParityGameGenerator {
  public:
    /**
     * @brief Main function for the parity game generator tool
     */
    static int run(int argc, char *argv[]) {
        try {
            po::options_description desc("Parity Game Generator Options");
            desc.add_options()("help,h", "Show help message");
            desc.add_options()("output-dir,o", po::value<std::string>()->required(), "Output directory for generated games");
            desc.add_options()("count,n", po::value<int>()->default_value(10), "Number of games to generate");
            desc.add_options()("vertices,v", po::value<int>()->default_value(10), "Number of vertices per game");
            desc.add_options()("max-priority", po::value<int>()->default_value(5), "Maximum priority for parity games");
            desc.add_options()("min-out-degree", po::value<int>()->default_value(1), "Minimum out-degree for each vertex");
            desc.add_options()("max-out-degree", po::value<int>(), "Maximum out-degree for each vertex (default: vertices-1)");
            desc.add_options()("seed,s", po::value<unsigned int>(), "Random seed (default: random)");
            desc.add_options()("verbose", "Show detailed output");

            po::variables_map vm;
            po::store(po::parse_command_line(argc, argv, desc), vm);

            if (vm.count("help")) {
                std::cout << desc << std::endl;
                return 0;
            }

            po::notify(vm);

            // Extract parameters
            const auto output_dir = vm["output-dir"].as<std::string>();
            const auto count = vm["count"].as<int>();
            const auto vertices = vm["vertices"].as<int>();
            const auto max_priority = vm["max-priority"].as<int>();
            const auto min_out_degree = vm["min-out-degree"].as<int>();
            const auto max_out_degree = vm.count("max-out-degree") ? vm["max-out-degree"].as<int>() : vertices - 1;
            const auto verbose = vm.count("verbose") > 0;

            // Validate parameters
            if (min_out_degree < 1) {
                std::cerr << "Error: min-out-degree must be at least 1" << std::endl;
                return 1;
            }
            if (max_out_degree < min_out_degree) {
                std::cerr << "Error: max-out-degree must be at least min-out-degree" << std::endl;
                return 1;
            }
            if (max_out_degree > vertices) {
                std::cerr << "Error: max-out-degree must be at most number of vertices (max: " << vertices << ")" << std::endl;
                return 1;
            }

            // Set up random seed
            const auto seed = vm.count("seed") ? vm["seed"].as<unsigned int>() : std::random_device{}();

            return generate_games(output_dir, count, vertices, max_priority, min_out_degree, max_out_degree, seed, verbose);

        } catch (const std::exception &e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
    }

  private:
    /**
     * @brief Generate random parity games
     */
    static int generate_games(const std::string &output_dir,
                              int count,
                              int vertices,
                              int max_priority,
                              int min_out_degree,
                              int max_out_degree,
                              unsigned int seed,
                              bool verbose) {

        // Create output directory
        try {
            fs::create_directories(output_dir);
        } catch (const fs::filesystem_error &e) {
            std::cerr << "Error creating output directory: " << e.what() << std::endl;
            return 1;
        }

        // Initialize random number generator
        std::mt19937 gen(seed);

        if (verbose) {
            std::cout << "Generating " << count << " parity games with "
                      << vertices << " vertices each" << std::endl;
            std::cout << "Out-degree range: [" << min_out_degree << ", " << max_out_degree << "]" << std::endl;
            std::cout << "Random seed: " << seed << std::endl;
            std::cout << "Output directory: " << output_dir << std::endl
                      << std::endl;
        }

        // Generate games
        for (int i = 0; i < count; ++i) {
            std::string filename = generate_game_file(output_dir, i, vertices, max_priority, min_out_degree, max_out_degree, gen);

            if (verbose) {
                std::cout << "Generated: " << filename << std::endl;
            }
        }

        if (verbose) {
            std::cout << std::endl
                      << "Successfully generated " << count << " parity games" << std::endl;
        }

        return 0;
    }

    /**
     * @brief Generate a single game file
     */
    static std::string generate_game_file(const std::string &output_dir,
                                          int game_index,
                                          int vertices,
                                          int max_priority,
                                          int min_out_degree,
                                          int max_out_degree,
                                          std::mt19937 &gen) {

        const auto filename = "parity_game_" + std::to_string(game_index + 1) + ".dot";
        const auto file_path = fs::path(output_dir) / filename;

        std::ofstream file(file_path.string());
        if (!file.is_open()) {
            throw std::runtime_error("Failed to create file: " + file_path.string());
        }

        generate_parity_game(file, vertices, max_priority, min_out_degree, max_out_degree, gen);
        file.close();
        return filename;
    }

    /**
     * @brief Generate a random parity game
     */
    static void generate_parity_game(std::ofstream &file,
                                     int vertices,
                                     int max_priority,
                                     int min_out_degree,
                                     int max_out_degree,
                                     std::mt19937 &gen) {

        std::uniform_int_distribution<int> player_dist(0, 1);
        std::uniform_int_distribution<int> priority_dist(0, max_priority);
        std::uniform_int_distribution<int> out_degree_dist(min_out_degree, max_out_degree);

        file << "digraph ParityGame {" << std::endl;

        // Generate vertices
        for (int i = 0; i < vertices; ++i) {
            const auto player = player_dist(gen);
            const auto priority = priority_dist(gen);

            file << "  v" << i << " [name=\"v" << i << "\", player="
                 << player << ", priority=" << priority << "];" << std::endl;
        }

        // Generate edges with controlled out-degrees
        for (int i = 0; i < vertices; ++i) {
            // Determine out-degree for this vertex
            const auto out_degree = out_degree_dist(gen);

            // Select unique target vertices (including self-loops)
            std::vector<int> available_targets;
            for (int j = 0; j < vertices; ++j) {
                available_targets.push_back(j);
            }

            // Shuffle and select first out_degree targets
            std::shuffle(available_targets.begin(), available_targets.end(), gen);

            // Take the first out_degree targets (or all if not enough available)
            int actual_degree = std::min(out_degree, static_cast<int>(available_targets.size()));

            for (int k = 0; k < actual_degree; ++k) {
                const auto target = available_targets[k];
                file << "  v" << i << " -> v" << target
                     << " [label=\"edge_" << i << "_" << target << "\"];" << std::endl;
            }
        }

        file << "}" << std::endl;
    }
};

namespace ggg_tools {
int run_generate_parity_games(int argc, char *argv[]) {
    return ParityGameGenerator::run(argc, argv);
}
} // namespace ggg_tools