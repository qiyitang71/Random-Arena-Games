#include "generate_discounted_games.hpp"
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

/**
 * @brief Tool to generate random discounted games for testing solvers
 *
 * This tool generates random discounted games in DOT format and saves them
 * to a specified directory.
 */
class DiscountedGameGenerator {
  public:
    /**
     * @brief Main function for the discounted game generator tool
     */
    static int run(int argc, char *argv[]) {
        try {
            po::options_description desc("Discounted Game Generator Options");
            desc.add_options()("help,h", "Display help message");
            desc.add_options()("count", po::value<int>(), "Number of games to generate");
            desc.add_options()("vertices,v", po::value<int>()->default_value(10), "Number of vertices per game");
            desc.add_options()("weight-min", po::value<double>()->default_value(-10.0), "Minimum weight for edge weights");
            desc.add_options()("weight-max", po::value<double>()->default_value(10.0), "Maximum weight for edge weights");
            desc.add_options()("discount-min", po::value<double>()->default_value(0.1), "Minimum discount factor (must be > 0)");
            desc.add_options()("discount-max", po::value<double>()->default_value(0.9), "Maximum discount factor (must be < 1)");
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
            const auto weight_min = vm["weight-min"].as<double>();
            const auto weight_max = vm["weight-max"].as<double>();
            const auto discount_min = vm["discount-min"].as<double>();
            const auto discount_max = vm["discount-max"].as<double>();
            const auto min_out_degree = vm["min-out-degree"].as<int>();
            const auto max_out_degree = vm.count("max-out-degree") ? vm["max-out-degree"].as<int>() : vertices - 1;
            const auto verbose = vm.count("verbose") > 0;

            // Validate parameters
            if (weight_min >= weight_max) {
                std::cerr << "Error: weight-min must be less than weight-max" << std::endl;
                return 1;
            }
            if (discount_min <= 0.0 || discount_min >= 1.0) {
                std::cerr << "Error: discount-min must be strictly between 0 and 1" << std::endl;
                return 1;
            }
            if (discount_max <= 0.0 || discount_max >= 1.0) {
                std::cerr << "Error: discount-max must be strictly between 0 and 1" << std::endl;
                return 1;
            }
            if (discount_min >= discount_max) {
                std::cerr << "Error: discount-min must be less than discount-max" << std::endl;
                return 1;
            }
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

            return generate_games(output_dir, count, vertices, weight_min, weight_max,
                                  discount_min, discount_max, min_out_degree, max_out_degree, seed, verbose);

        } catch (const std::exception &e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
    }

  private:
    /**
     * @brief Generate random discounted games
     */
    static int generate_games(const std::string &output_dir,
                              int count,
                              int vertices,
                              double weight_min,
                              double weight_max,
                              double discount_min,
                              double discount_max,
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
            std::cout << "Generating " << count << " discounted games with "
                      << vertices << " vertices each" << std::endl;
            std::cout << "Weight range: [" << weight_min << ", " << weight_max << "]" << std::endl;
            std::cout << "Discount range: [" << discount_min << ", " << discount_max << "]" << std::endl;
            std::cout << "Out-degree range: [" << min_out_degree << ", " << max_out_degree << "]" << std::endl;
            std::cout << "Random seed: " << seed << std::endl;
            std::cout << "Output directory: " << output_dir << std::endl
                      << std::endl;
        }

        // Generate games
        for (int i = 0; i < count; ++i) {
            std::string filename = generate_game_file(output_dir, i, vertices,
                                                      weight_min, weight_max,
                                                      discount_min, discount_max,
                                                      min_out_degree, max_out_degree, gen);

            if (verbose) {
                std::cout << "Generated: " << filename << std::endl;
            }
        }

        if (verbose) {
            std::cout << std::endl
                      << "Successfully generated " << count << " discounted games" << std::endl;
        }

        return 0;
    }

    /**
     * @brief Generate a single game file
     */
    static std::string generate_game_file(const std::string &output_dir,
                                          int game_index,
                                          int vertices,
                                          double weight_min,
                                          double weight_max,
                                          double discount_min,
                                          double discount_max,
                                          int min_out_degree,
                                          int max_out_degree,
                                          std::mt19937 &gen) {

        const auto filename = "discounted_game_" + std::to_string(game_index + 1) + ".dot";
        const auto file_path = fs::path(output_dir) / filename;

        std::ofstream file(file_path.string());
        if (!file.is_open()) {
            throw std::runtime_error("Failed to create file: " + file_path.string());
        }

        generate_discounted_game(file, vertices, weight_min, weight_max,
                                 discount_min, discount_max, min_out_degree, max_out_degree, gen);
        file.close();
        return filename;
    }

    /**
     * @brief Generate a random discounted game
     */
    static void generate_discounted_game(std::ofstream &file,
                                         int vertices,
                                         double weight_min,
                                         double weight_max,
                                         double discount_min,
                                         double discount_max,
                                         int min_out_degree,
                                         int max_out_degree,
                                         std::mt19937 &gen) {

        std::uniform_int_distribution<int> player_dist(0, 1);
        std::uniform_real_distribution<double> weight_dist(weight_min, weight_max);
        std::uniform_real_distribution<double> discount_dist(discount_min, discount_max);
        std::uniform_int_distribution<int> out_degree_dist(min_out_degree, max_out_degree);

        file << "digraph DiscountedGame {" << std::endl;

        // Generate vertices
        for (int i = 0; i < vertices; ++i) {
            const auto player = player_dist(gen);

            file << "  v" << i << " [name=\"v" << i << "\", player="
                 << player << "];" << std::endl;
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
                const auto weight = weight_dist(gen);
                const auto discount = discount_dist(gen);

                file << "  v" << i << " -> v" << target
                     << " [label=\"edge_" << i << "_" << target
                     << "\", weight=" << std::fixed << std::setprecision(6) << weight
                     << ", discount=" << std::fixed << std::setprecision(6) << discount
                     << "];" << std::endl;
            }
        }

        file << "}" << std::endl;
    }
};

namespace ggg_tools {
int run_generate_discounted_games(int argc, char *argv[]) {
    return DiscountedGameGenerator::run(argc, argv);
}
} // namespace ggg_tools