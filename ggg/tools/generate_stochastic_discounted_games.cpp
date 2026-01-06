#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <vector>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

/**
 * @brief Tool to generate random stochastic discounted games for testing solvers
 *
 * This tool generates random stochastic discounted games in DOT format and saves them
 * to a specified directory. The games include probabilistic vertices (player -1) in addition
 * to normal players 0 and 1.
 */
class StochasticDiscountedGameGenerator {
  public:
    /**
     * @brief Main function for the stochastic discounted game generator tool
     */
    static int run(int argc, char *argv[]) {
        try {
            po::options_description desc("Stochastic Discounted Game Generator Options");
            desc.add_options()("help,h", "Show help message");
            desc.add_options()("output-dir,o", po::value<std::string>()->required(), "Output directory for generated games");
            desc.add_options()("count,n", po::value<int>()->default_value(10), "Number of games to generate");
            desc.add_options()("vertices,v", po::value<int>()->default_value(10), "Number of vertices per game");
            desc.add_options()("weight-min", po::value<double>()->default_value(-10.0), "Minimum weight for edge weights");
            desc.add_options()("weight-max", po::value<double>()->default_value(10.0), "Maximum weight for edge weights");
            desc.add_options()("discount-min", po::value<double>()->default_value(0.1), "Minimum discount factor (must be > 0)");
            desc.add_options()("discount-max", po::value<double>()->default_value(0.9), "Maximum discount factor (must be < 1)");
            desc.add_options()("prob-vertices-ratio", po::value<double>()->default_value(0.3), "Ratio of probabilistic vertices (0.0-1.0)");
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
            const auto prob_vertices_ratio = vm["prob-vertices-ratio"].as<double>();
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
            if (prob_vertices_ratio >= 1.0) {
                std::cerr << "Error: prob-vertices-ratio must be less than 1.0 to ensure non-probabilistic targets exist" << std::endl;
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
                                  discount_min, discount_max, prob_vertices_ratio, min_out_degree, max_out_degree, seed, verbose);

        } catch (const std::exception &e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
    }

  private:
    /**
     * @brief Generate random stochastic discounted games
     */
    static int generate_games(const std::string &output_dir,
                              int count,
                              int vertices,
                              double weight_min,
                              double weight_max,
                              double discount_min,
                              double discount_max,
                              double prob_vertices_ratio,
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
            std::cout << "Generating " << count << " stochastic discounted games with "
                      << vertices << " vertices each" << std::endl;
            std::cout << "Weight range: [" << weight_min << ", " << weight_max << "]" << std::endl;
            std::cout << "Discount range: [" << discount_min << ", " << discount_max << "]" << std::endl;
            std::cout << "Probabilistic vertices ratio: " << prob_vertices_ratio << std::endl;
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
                                                      prob_vertices_ratio, min_out_degree, max_out_degree, gen);

            if (verbose) {
                std::cout << "Generated: " << filename << std::endl;
            }
        }

        if (verbose) {
            std::cout << std::endl
                      << "Successfully generated " << count << " stochastic discounted games" << std::endl;
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
                                          double prob_vertices_ratio,
                                          int min_out_degree,
                                          int max_out_degree,
                                          std::mt19937 &gen) {

        const auto filename = "stochastic_discounted_game_" + std::to_string(game_index + 1) + ".dot";
        const auto file_path = fs::path(output_dir) / filename;

        std::ofstream file(file_path.string());
        if (!file.is_open()) {
            throw std::runtime_error("Failed to create file: " + file_path.string());
        }

        generate_stochastic_discounted_game(file, vertices, weight_min, weight_max,
                                            discount_min, discount_max, prob_vertices_ratio, min_out_degree, max_out_degree, gen);
        file.close();
        return filename;
    }

    /**
     * @brief Generate a random stochastic discounted game
     */
    static void generate_stochastic_discounted_game(std::ofstream &file,
                                                    int vertices,
                                                    double weight_min,
                                                    double weight_max,
                                                    double discount_min,
                                                    double discount_max,
                                                    double prob_vertices_ratio,
                                                    int min_out_degree,
                                                    int max_out_degree,
                                                    std::mt19937 &gen) {

        std::uniform_int_distribution<int> player_dist(0, 1);
        std::uniform_real_distribution<double> weight_dist(weight_min, weight_max);
        std::uniform_real_distribution<double> discount_dist(discount_min, discount_max);
        std::uniform_int_distribution<int> out_degree_dist(min_out_degree, max_out_degree);
        std::uniform_real_distribution<double> prob_dist(0.0, 1.0);

        file << "digraph StochasticDiscountedGame {" << std::endl;

        // Determine which vertices are probabilistic
        int prob_vertices = static_cast<int>(vertices * prob_vertices_ratio);
        std::vector<int> vertex_types(vertices);

        // Fill with normal players first
        for (int i = 0; i < vertices; ++i) {
            vertex_types[i] = player_dist(gen); // 0 or 1
        }

        // Convert some to probabilistic (-1)
        std::vector<int> indices(vertices);
        std::iota(indices.begin(), indices.end(), 0);
        std::shuffle(indices.begin(), indices.end(), gen);

        for (int i = 0; i < prob_vertices; ++i) {
            vertex_types[indices[i]] = -1;
        }

        // Generate vertices
        for (int i = 0; i < vertices; ++i) {
            file << "  v" << i << " [name=\"v" << i << "\", player="
                 << vertex_types[i] << "];" << std::endl;
        }

        // Generate edges with controlled out-degrees
        for (int i = 0; i < vertices; ++i) {
            // Determine out-degree for this vertex
            const auto out_degree = out_degree_dist(gen);

            // Select unique target vertices
            std::vector<int> available_targets;

            if (vertex_types[i] == -1) {
                // Probabilistic vertex: can only target non-probabilistic vertices to avoid cycles
                for (int j = 0; j < vertices; ++j) {
                    if (vertex_types[j] != -1) {
                        available_targets.push_back(j);
                    }
                }
            } else {
                // Non-probabilistic vertex: can target any vertex
                for (int j = 0; j < vertices; ++j) {
                    available_targets.push_back(j);
                }
            }

            // Check if we have enough targets
            if (available_targets.empty()) {
                // If a probabilistic vertex has no non-probabilistic targets,
                // we need to create at least one non-probabilistic vertex
                // For now, just skip this vertex's edges (shouldn't happen with proper ratio)
                continue;
            }

            // Shuffle and select targets
            std::shuffle(available_targets.begin(), available_targets.end(), gen);

            // Take the first OUT_DEGREE targets (or all available if less)
            int actual_degree = std::min(out_degree, static_cast<int>(available_targets.size()));
            std::vector<int> targets(available_targets.begin(), available_targets.begin() + actual_degree);

            if (vertex_types[i] == -1) {
                // Probabilistic vertex: edges have probabilities that sum to 1
                // Use integer arithmetic to avoid floating point precision issues
                std::vector<int> int_probs(actual_degree);
                int total_parts = 0;

                // Generate random integer parts (making sure we have at least 1 for each)
                for (int k = 0; k < actual_degree - 1; ++k) {
                    int_probs[k] = 1 + (static_cast<int>(prob_dist(gen) * 1000)) % 100; // Random 1-100
                    total_parts += int_probs[k];
                }

                // Set the last part to ensure we have enough for normalization
                int_probs[actual_degree - 1] = std::max(1, 100 - (total_parts % 100));
                total_parts += int_probs[actual_degree - 1];

                // Convert to probabilities that sum exactly to 1.0
                std::vector<double> probs(actual_degree);
                for (int k = 0; k < actual_degree; ++k) {
                    probs[k] = static_cast<double>(int_probs[k]) / static_cast<double>(total_parts);
                }

                for (int k = 0; k < actual_degree; ++k) {
                    const auto target = targets[k];
                    file << "  v" << i << " -> v" << target
                         << " [label=\"edge_" << i << "_" << target
                         << "\", probability=" << std::fixed << std::setprecision(10) << probs[k]
                         << "];" << std::endl;
                }
            } else {
                // Normal vertex: edges have weights and discounts
                for (int k = 0; k < actual_degree; ++k) {
                    const auto target = targets[k];
                    const auto weight = weight_dist(gen);
                    const auto discount = discount_dist(gen);

                    file << "  v" << i << " -> v" << target
                         << " [label=\"edge_" << i << "_" << target
                         << "\", weight=" << std::fixed << std::setprecision(6) << weight
                         << ", discount=" << std::fixed << std::setprecision(6) << discount
                         << "];" << std::endl;
                }
            }
        }

        file << "}" << std::endl;
    }
};

int main(int argc, char *argv[]) {
    return StochasticDiscountedGameGenerator::run(argc, argv);
}