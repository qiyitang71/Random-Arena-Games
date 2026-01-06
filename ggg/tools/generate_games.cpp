#include "generate_games.hpp"
#include <boost/program_options.hpp>
#include <iostream>
#include <string>
#include <vector>
#include "generate_discounted_games.hpp"
#include "generate_mpv_games.hpp"
#include "generate_parity_games.hpp"

namespace po = boost::program_options;

namespace ggg_tools {

int run_generate_games(int argc, char *argv[]) {
    try {
        po::options_description desc("Generate Options");
        desc.add_options()("help,h", "Show help message");
        desc.add_options()("type,t", po::value<std::string>()->required(), "Game type (parity, meanpayoff, discounted)");
        desc.add_options()("output-dir,o", po::value<std::string>()->required(), "Output directory for generated games");
        desc.add_options()("count,n", po::value<int>()->default_value(10), "Number of games to generate");
        desc.add_options()("vertices,v", po::value<int>()->default_value(10), "Number of vertices per game");
        desc.add_options()("max-priority", po::value<int>()->default_value(5), "Maximum priority (parity games only)");
        desc.add_options()("max-weight", po::value<int>()->default_value(10), "Maximum weight (meanpayoff/discounted games only)");
        desc.add_options()("min-out-degree", po::value<int>()->default_value(1), "Minimum out-degree for each vertex");
        desc.add_options()("max-out-degree", po::value<int>(), "Maximum out-degree for each vertex");
        desc.add_options()("discount", po::value<double>()->default_value(0.9), "Discount factor (discounted games only)");
        desc.add_options()("seed,s", po::value<unsigned int>(), "Random seed (default: random)");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);

        if (vm.count("help")) {
            std::cout << "ggg generate - Generate random game graphs\n\n";
            std::cout << desc << std::endl;
            return 0;
        }

        po::notify(vm);

        std::string type = vm["type"].as<std::string>();

        // Create args for the specific generator
        std::vector<char *> generator_args;

        if (type == "parity") {
            generator_args.push_back(const_cast<char *>("ggg generate-parity"));
        } else if (type == "meanpayoff") {
            generator_args.push_back(const_cast<char *>("ggg generate-mpv"));
        } else if (type == "discounted") {
            generator_args.push_back(const_cast<char *>("ggg generate-discounted"));
        } else {
            std::cerr << "Error: Unknown game type '" << type << "'. Available types: parity, meanpayoff, discounted" << std::endl;
            return 1;
        }

        // Add common parameters
        if (vm.count("output-dir")) {
            generator_args.push_back(const_cast<char *>("--output-dir"));
            generator_args.push_back(const_cast<char *>(vm["output-dir"].as<std::string>().c_str()));
        }

        if (vm.count("count")) {
            generator_args.push_back(const_cast<char *>("--count"));
            std::string count_str = std::to_string(vm["count"].as<int>());
            generator_args.push_back(const_cast<char *>(count_str.c_str()));
        }

        if (vm.count("vertices")) {
            generator_args.push_back(const_cast<char *>("--vertices"));
            std::string vertices_str = std::to_string(vm["vertices"].as<int>());
            generator_args.push_back(const_cast<char *>(vertices_str.c_str()));
        }

        if (vm.count("min-out-degree")) {
            generator_args.push_back(const_cast<char *>("--min-out-degree"));
            std::string min_out_degree_str = std::to_string(vm["min-out-degree"].as<int>());
            generator_args.push_back(const_cast<char *>(min_out_degree_str.c_str()));
        }

        if (vm.count("max-out-degree")) {
            generator_args.push_back(const_cast<char *>("--max-out-degree"));
            std::string max_out_degree_str = std::to_string(vm["max-out-degree"].as<int>());
            generator_args.push_back(const_cast<char *>(max_out_degree_str.c_str()));
        }

        // Game-specific parameters
        if (type == "parity" && vm.count("max-priority")) {
            generator_args.push_back(const_cast<char *>("--max-priority"));
            std::string max_priority_str = std::to_string(vm["max-priority"].as<int>());
            generator_args.push_back(const_cast<char *>(max_priority_str.c_str()));
        }

        if ((type == "meanpayoff" || type == "discounted") && vm.count("max-weight")) {
            generator_args.push_back(const_cast<char *>("--max-weight"));
            std::string max_weight_str = std::to_string(vm["max-weight"].as<int>());
            generator_args.push_back(const_cast<char *>(max_weight_str.c_str()));
        }

        if (type == "discounted" && vm.count("discount")) {
            generator_args.push_back(const_cast<char *>("--discount"));
            std::string discount_str = std::to_string(vm["discount"].as<double>());
            generator_args.push_back(const_cast<char *>(discount_str.c_str()));
        }

        if (vm.count("seed")) {
            generator_args.push_back(const_cast<char *>("--seed"));
            std::string seed_str = std::to_string(vm["seed"].as<unsigned int>());
            generator_args.push_back(const_cast<char *>(seed_str.c_str()));
        }

        // Call the appropriate generator function
        if (type == "parity") {
            return run_generate_parity_games(generator_args.size(), generator_args.data());
        } else if (type == "meanpayoff") {
            return run_generate_mpv_games(generator_args.size(), generator_args.data());
        } else if (type == "discounted") {
            return run_generate_discounted_games(generator_args.size(), generator_args.data());
        }

        return 1;

    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

} // namespace ggg_tools