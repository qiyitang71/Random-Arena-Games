#pragma once

#include "libggg/solvers/solver.hpp"
#include "libggg/utils/logging.hpp"
#include <boost/graph/adjacency_list.hpp>
#include <boost/program_options.hpp>
#include <chrono>
#include <concepts>
#include <iostream>
#include <memory>
#include <vector>

namespace ggg {
namespace utils {

// C++20 concept to ensure solver has solve method
template <typename SolverType, typename GraphType>
concept HasSolveMethod = requires(SolverType solver, const GraphType &graph) {
    solver.solve(graph);
};

// C++20 concept to detect solutions with generic statistics
template <typename SolutionType>
concept HasStatistics = requires(const SolutionType &solution) {
    { solution.get_statistics() } -> std::convertible_to<std::map<std::string, std::string>>;
};

/**
 * @brief Generic wrapper for game solvers
 * @template GraphType The graph type (ParityGraph, MeanPayoffGraph)
 * @template SolverType The solver class for the graph type
 */
template <typename GraphType, typename SolverType>
class GameSolverWrapper {
  private:
    /**
     * @brief Parse command line options
     */
    static boost::program_options::variables_map parse_command_line(int argc, char *argv[]) {
        boost::program_options::options_description desc("Solver Options");
        desc.add_options()("help,h", "Show help message");
        desc.add_options()("csv", "Output in CSV format");
        desc.add_options()("input,i", boost::program_options::value<std::string>()->default_value("-"), "Input file (default: stdin)");
        desc.add_options()("output,o", boost::program_options::value<std::string>()->default_value("-"), "Output file (default: stdout)");
        desc.add_options()("time-only,t", "Only output time to solve (in milliseconds)");
        desc.add_options()("solver-name", "Output solver name");

        // Add positional argument for input file
        boost::program_options::positional_options_description pos_desc;
        pos_desc.add("input", 1);

#ifdef ENABLE_LOGGING
        desc.add_options()("verbose,v", boost::program_options::value<std::vector<std::string>>()->multitoken()->zero_tokens(),
                           "Increase verbosity (can be used multiple times: -v, -vv, -vvv)");
#endif

        boost::program_options::variables_map vm;

        // Parse manually to handle multiple -v flags and -vv, -vvv style flags
        std::vector<std::string> args;
        int verbosity = 0;

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "-v" || arg == "--verbose") {
                verbosity++;
            } else if (arg.substr(0, 2) == "-v" && arg.length() > 2) {
                // Count multiple v's in -vvv style
                verbosity = static_cast<int>(arg.length()) - 1;
            } else {
                args.push_back(arg);
            }
        }

        // Create modified argv without -v flags for boost parsing
        std::vector<char *> new_argv;
        new_argv.push_back(argv[0]);
        for (const auto &arg : args) {
            new_argv.push_back(const_cast<char *>(arg.c_str()));
        }

        boost::program_options::store(
            boost::program_options::command_line_parser(static_cast<int>(new_argv.size()), new_argv.data())
                .options(desc)
                .positional(pos_desc)
                .run(),
            vm);
        boost::program_options::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << std::endl;
            exit(0);
        }

#ifdef ENABLE_LOGGING
        if (verbosity > 0) {
            set_log_level(verbosity_to_log_level(verbosity));
            LGG_INFO("Logging level set to verbosity ", verbosity);
        }
#endif

        return vm;
    }

    /**
     * @brief Output solution in CSV format
     */
    template <typename SolutionType>
    static void output_csv(const GraphType &graph,
                           const SolutionType &solution,
                           double time_seconds)
        requires solvers::HasRegions<SolutionType, GraphType> && solvers::HasStrategy<SolutionType, GraphType>
    {
        using Vertex = typename boost::graph_traits<GraphType>::vertex_descriptor;

        // Get statistics if available for header
        std::map<std::string, std::string> stats;
        if constexpr (HasStatistics<SolutionType>) {
            stats = solution.get_statistics();
        }

        // CSV header - include value column if solution has quantitative values
        // and statistics columns if solution has statistics
        std::cout << "vertex,player,winning_player,strategy";

        if constexpr (solvers::HasValueMapping<SolutionType, GraphType>) {
            std::cout << ",value";
        }

        std::cout << ",solve_time";

        if constexpr (HasStatistics<SolutionType>) {
            for (const auto &[key, value] : stats) {
                std::cout << "," << key;
            }
        }

        std::cout << std::endl;

        // CSV data rows
        auto vertices = boost::vertices(graph);
        for (auto it = vertices.first; it != vertices.second; ++it) {
            Vertex vertex = *it;
            std::string vertex_name = graph[vertex].name;
            int vertex_player = graph[vertex].player;

            int winning_player = -1;
            if (solution.is_won_by_player0(vertex)) {
                winning_player = 0;
            } else if (solution.is_won_by_player1(vertex)) {
                winning_player = 1;
            }

            std::string strategy = "";
            auto strategy_vertex = solution.get_strategy(vertex);
            if (strategy_vertex != boost::graph_traits<GraphType>::null_vertex()) {
                strategy = graph[strategy_vertex].name;
            }

            std::cout << vertex_name << ","
                      << vertex_player << ","
                      << winning_player << ","
                      << strategy;

            // Include value if available
            if constexpr (solvers::HasValueMapping<SolutionType, GraphType>) {
                std::cout << ",";
                if (solution.has_value(vertex)) {
                    std::cout << solution.get_value(vertex);
                } else {
                    std::cout << "";
                }
            }

            std::cout << "," << time_seconds;

            // Include statistics if available
            if constexpr (HasStatistics<SolutionType>) {
                for (const auto &[key, value] : stats) {
                    std::cout << "," << value;
                }
            }

            std::cout << std::endl;
        }
    }

    /**
     * @brief Output solution in human-readable format
     * TODO:instead of printing to stdout, make this return a string representation of the solution
     * (and put this function somewhere more apropriate)
     */
    template <typename SolutionType>
    static void output_human(const GraphType &graph,
                             const SolutionType &solution,
                             double time_to_solve)
        requires solvers::HasRegions<SolutionType, GraphType> && solvers::HasStrategy<SolutionType, GraphType>
    {
        using Vertex = typename boost::graph_traits<GraphType>::vertex_descriptor;

        std::cout << "Time to solve: " << time_to_solve << " ms" << std::endl;
        std::cout << "Solution:" << std::endl;

        auto vertices = boost::vertices(graph);
        for (auto it = vertices.first; it != vertices.second; ++it) {
            Vertex vertex = *it;
            std::string vertex_name = graph[vertex].name;

            std::cout << "  " << vertex_name << ": ";
            if (solution.is_won_by_player0(vertex)) {
                std::cout << "Player 0";
            } else if (solution.is_won_by_player1(vertex)) {
                std::cout << "Player 1";
            } else {
                std::cout << "Unknown";
            }

            auto strategy_vertex = solution.get_strategy(vertex);
            if (strategy_vertex != boost::graph_traits<GraphType>::null_vertex()) {
                std::cout << " -> " << graph[strategy_vertex].name;
            }

            // Print quantitative values if available
            if constexpr (solvers::HasValueMapping<SolutionType, GraphType>) {
                if (solution.has_value(vertex)) {
                    std::cout << " (value: " << solution.get_value(vertex) << ")";
                }
            }

            std::cout << std::endl;
        }

        // Print statistics if available
        if constexpr (HasStatistics<SolutionType>) {
            auto stats = solution.get_statistics();
            if (!stats.empty()) {
                std::cout << "Statistics:" << std::endl;
                for (const auto &[key, value] : stats) {
                    std::cout << "  " << key << ": " << value << std::endl;
                }
            }
        }
    }

  public:
    template <typename ParserFunc>
    static int run(int argc, char *argv[], ParserFunc parser_func) {
        try {
            auto vm = parse_command_line(argc, argv);

            LGG_DEBUG("Starting GameSolverWrapper");

            if (vm.count("solver-name")) {
                SolverType solver;
                std::cout << solver.get_name() << std::endl;
                return 0;
            }

            // Parse input game
            std::string input_file = vm["input"].as<std::string>();
            std::shared_ptr<GraphType> graph;

            LGG_INFO("Parsing input from: ", (input_file == "-" ? "stdin" : input_file));

            if (input_file == "-") {
                graph = parser_func(std::cin);
            } else {
                graph = parser_func(input_file);
            }

            if (!graph) {
                LGG_ERROR("Failed to parse input game");
                std::cerr << "Error: Failed to parse input game" << std::endl;
                return 1;
            }

            LGG_INFO("Successfully parsed game with ", boost::num_vertices(*graph), " vertices");

            // Create solver and measure time
            SolverType solver;
            LGG_DEBUG("Starting solver: ", solver.get_name());

            static_assert(HasSolveMethod<SolverType, GraphType>,
                          "Solver must have solve() method");

            auto start = std::chrono::high_resolution_clock::now();
            auto solution = solver.solve(*graph);
            auto end = std::chrono::high_resolution_clock::now();

            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            double time_to_solve = duration.count() / 1000.0;

            LGG_DEBUG("Solver completed in ", time_to_solve, " milliseconds");

            if (!solution.is_solved()) {
                LGG_ERROR("Solver failed to solve the game");
                std::cerr << "Error: Failed to solve game" << std::endl;
                return 1;
            }

            LGG_INFO("Game solved successfully");

            // Output results
            if (vm.count("time-only")) {
                std::cout << "Time to solve: " << time_to_solve << " ms" << std::endl;
            } else {
                if (vm.count("csv")) {
                    output_csv(*graph, solution, time_to_solve);
                } else {
                    output_human(*graph, solution, time_to_solve);
                }
            }

            return 0;

        } catch (const std::exception &e) {
            LGG_ERROR("Exception caught: ", e.what());
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
    }
};

/**
 * @brief Macro to create main functions for game solvers
 * @param GraphType The game graph type (e.g., graphs::ParityGraph)
 * @param ParserFuncName The parser function name (e.g., parse_Parity_graph)
 * @param SolverType The solver class
 */
#define GGG_GAME_SOLVER_MAIN(GraphType, ParserFuncName, SolverType)                                \
    int main(int argc, char *argv[]) {                                                             \
        auto parser_func = [](auto &&input) { return ParserFuncName(input); };                     \
        return ggg::utils::GameSolverWrapper<GraphType, SolverType>::run(argc, argv, parser_func); \
    }

} // namespace utils
} // namespace ggg
