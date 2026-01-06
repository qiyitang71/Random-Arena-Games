#pragma once

#include <boost/graph/graph_traits.hpp>
#include <concepts>
#include <map>
#include <string>
#include <vector>

namespace ggg {
namespace solvers {

// =============================================================================
// C++20 Concepts for Solution Capabilities
// =============================================================================

/**
 * @brief C++20 concept for solution types that provide winning regions
 *
 * Solutions satisfying this concept can determine which player wins each vertex
 * in the game graph.
 */
template <typename SolutionType, typename GraphType>
concept HasRegions = requires(const SolutionType &solution,
                              typename boost::graph_traits<GraphType>::vertex_descriptor vertex) {
    { solution.is_won_by_player0(vertex) } -> std::convertible_to<bool>;
    { solution.is_won_by_player1(vertex) } -> std::convertible_to<bool>;
    { solution.get_winning_player(vertex) } -> std::convertible_to<int>;
};

// =============================================================================
// Strategy Type Concepts and Definitions
// =============================================================================

/**
 * @brief Deterministic positional strategy: vertex -> vertex
 */
template <typename GraphType>
using DeterministicStrategy = typename boost::graph_traits<GraphType>::vertex_descriptor;

/**
 * @brief Mixing strategy: vertex -> distribution over vertices
 * Each vertex maps to a vector of (successor, probability) pairs
 */
template <typename GraphType>
using MixingStrategy = std::vector<std::pair<typename boost::graph_traits<GraphType>::vertex_descriptor, double>>;

/**
 * @brief Finite memory strategy: (vertex, memory_state) -> (vertex, new_memory_state)
 * Memory state is represented as an integer
 */
template <typename GraphType>
using FiniteMemoryStrategy = std::pair<typename boost::graph_traits<GraphType>::vertex_descriptor, int>;

/**
 * @brief C++20 concept for deterministic strategy types
 */
template <typename StrategyType, typename GraphType>
concept DeterministicStrategyType = std::same_as<StrategyType, DeterministicStrategy<GraphType>>;

/**
 * @brief C++20 concept for mixing strategy types
 */
template <typename StrategyType, typename GraphType>
concept MixingStrategyType = std::same_as<StrategyType, MixingStrategy<GraphType>>;

/**
 * @brief C++20 concept for finite memory strategy types
 */
template <typename StrategyType, typename GraphType>
concept FiniteMemoryStrategyType = std::same_as<StrategyType, FiniteMemoryStrategy<GraphType>>;

/**
 * @brief C++20 concept for valid strategy types
 */
template <typename StrategyType, typename GraphType>
concept ValidStrategyType = DeterministicStrategyType<StrategyType, GraphType> ||
                            MixingStrategyType<StrategyType, GraphType> ||
                            FiniteMemoryStrategyType<StrategyType, GraphType>;

/**
 * @brief C++20 concept for solution types that provide strategic choices
 *
 * Solutions satisfying this concept can provide optimal strategic moves for vertices.
 */
template <typename SolutionType, typename GraphType, typename StrategyType = DeterministicStrategy<GraphType>>
concept HasStrategy = requires(const SolutionType &solution,
                               typename boost::graph_traits<GraphType>::vertex_descriptor vertex) {
    { solution.get_strategy(vertex) } -> std::convertible_to<StrategyType>;
    { solution.has_strategy(vertex) } -> std::convertible_to<bool>;
} && ValidStrategyType<StrategyType, GraphType>;

/**
 * @brief C++20 concept for solution types that provide value mapping
 *
 * Solutions satisfying this concept can provide numerical values associated with vertices,
 * useful for mean payoff games or other quantitative game types.
 */
template <typename SolutionType, typename GraphType, typename ValueType = double>
concept HasValueMapping = requires(const SolutionType &solution,
                                   typename boost::graph_traits<GraphType>::vertex_descriptor vertex) {
    { solution.get_value(vertex) } -> std::convertible_to<ValueType>;
    { solution.has_value(vertex) } -> std::convertible_to<bool>;
};

/**
 * @brief C++20 concept for solution types that provide initial state status
 *
 * Solutions satisfying this concept can indicate whether the solution is complete
 * and valid.
 */
template <typename SolutionType>
concept HasInitialStateStatus = requires(const SolutionType &solution) {
    { solution.is_solved() } -> std::convertible_to<bool>;
    { solution.isValid() } -> std::convertible_to<bool>;
};

// =============================================================================
// Solution Types
// =============================================================================

/**
 * @brief Base solution providing only initial state status (I capability)
 */
class ISolution {
  protected:
    bool solved_ = false;
    bool valid_ = true;

  public:
    ISolution() = default;
    explicit ISolution(bool solved, bool valid = true) : solved_(solved), valid_(valid) {}

    /**
     * @brief Check if the solution process completed successfully
     * @return True if the game was fully solved
     */
    bool is_solved() const { return solved_; }

    /**
     * @brief Check if the solution is valid and consistent
     * @return True if the solution represents a valid game state
     */
    bool is_valid() const { return valid_; }

    /**
     * @brief Mark solution as solved
     */
    void set_solved(bool solved) { solved_ = solved; }

    /**
     * @brief Mark solution as valid/invalid
     */
    void set_valid(bool valid) { valid_ = valid; }
};

/**
 * @brief Solution providing winning regions capability (R capability)
 * @template GraphType The graph type (ParityGraph, MeanPayoffGraph)
 */
template <typename GraphType>
class RSolution : public virtual ISolution {
  public:
    using Vertex = typename boost::graph_traits<GraphType>::vertex_descriptor;

  protected:
    std::map<Vertex, int> winning_regions_;

  public:
    RSolution() = default;
    explicit RSolution(bool solved, bool valid = true) : ISolution(solved, valid) {}

    /**
     * @brief Check if a vertex is in player 0's winning region
     * @param vertex Vertex to check
     * @return True if vertex is won by player 0
     */
    bool is_won_by_player0(Vertex vertex) const {
        const auto it = winning_regions_.find(vertex);
        return it != winning_regions_.end() && it->second == 0;
    }

    /**
     * @brief Check if a vertex is in player 1's winning region
     * @param vertex Vertex to check
     * @return True if vertex is won by player 1
     */
    bool is_won_by_player1(Vertex vertex) const {
      const auto it = winning_regions_.find(vertex);
      return it != winning_regions_.end() && it->second == 1;
    }

    /**
     * @brief Get the winning player for a vertex
     * @param vertex Vertex to check
     * @return Winning player (0 or 1), or -1 if undetermined
     */
    int get_winning_player(Vertex vertex) const {
        const auto it = winning_regions_.find(vertex);
        return it != winning_regions_.end() ? it->second : -1;
    }

    /**
     * @brief Set the winning player for a vertex
     * @param vertex Vertex to set
     * @param player Winning player (0 or 1)
     */
    void set_winning_player(Vertex vertex, int player) {
        winning_regions_[vertex] = player;
    }

    /**
     * @brief Get all winning regions
     * @return Map from vertex to winning player
     */
    const std::map<Vertex, int> &get_winning_regions() const { return winning_regions_; }
};

/**
 * @brief Solution providing strategy capability (S capability)
 * @template GraphType The graph type (ParityGraph, MeanPayoffGraph)
 * @template StrategyType The strategy type (default: DeterministicStrategy)
 */
template <typename GraphType, typename StrategyType = DeterministicStrategy<GraphType>>
class SSolution : public virtual ISolution {
  public:
    using Vertex = typename boost::graph_traits<GraphType>::vertex_descriptor;
    using Strategy = StrategyType;

  protected:
    std::map<Vertex, StrategyType> strategy_;

  public:
    SSolution() = default;
    explicit SSolution(bool solved, bool valid = true) : ISolution(solved, valid) {}

    /**
     * @brief Get the strategic choice for a vertex
     * @param vertex Vertex to get strategy for
     * @return Strategic choice for the vertex, or null_vertex()/default if none
     */
    StrategyType get_strategy(Vertex vertex) const {
        const auto it = strategy_.find(vertex);
        if (it != strategy_.end()) {
            return it->second;
        }

        if constexpr (DeterministicStrategyType<StrategyType, GraphType>) {
            return boost::graph_traits<GraphType>::null_vertex();
        } else {
            return StrategyType{};
        }
    }

    /**
     * @brief Check if a vertex has a strategic choice
     * @param vertex Vertex to check
     * @return True if vertex has a strategy defined
     */
    bool has_strategy(Vertex vertex) const {
        return strategy_.find(vertex) != strategy_.end();
    }

    /**
     * @brief Set the strategic choice for a vertex
     * @param vertex Source vertex
     * @param strategy Strategic choice to assign
     */
    void set_strategy(Vertex vertex, const StrategyType &strategy) {
        strategy_[vertex] = strategy;
    }

    /**
     * @brief Get all strategic choices
     * @return Map from vertex to strategic choice
     */
    const std::map<Vertex, StrategyType> &get_strategies() const { return strategy_; }
};

/**
 * @brief Solution providing quantitative values capability (Q capability)
 * @template GraphType The graph type (ParityGraph, MeanPayoffGraph)
 * @template ValueType The type of values associated with vertices (default: double)
 */
template <typename GraphType, typename ValueType = double>
class QSolution : public virtual ISolution {
  public:
    using Vertex = typename boost::graph_traits<GraphType>::vertex_descriptor;
    using Value = ValueType;

  protected:
    std::map<Vertex, ValueType> values_;

  public:
    QSolution() = default;
    explicit QSolution(bool solved, bool valid = true) : ISolution(solved, valid) {}

    /**
     * @brief Get the value associated with a vertex
     * @param vertex Vertex to get value for
     * @return Numerical value, or default-constructed ValueType if no value is set
     */
    ValueType get_value(Vertex vertex) const {
        const auto it = values_.find(vertex);
        return it != values_.end() ? it->second : ValueType{};
    }

    /**
     * @brief Check if a vertex has a value assigned
     * @param vertex Vertex to check
     * @return True if vertex has a value defined
     */
    bool has_value(Vertex vertex) const {
        return values_.find(vertex) != values_.end();
    }

    /**
     * @brief Set the value for a vertex
     * @param vertex Vertex to set value for
     * @param value Numerical value to assign
     */
    void set_value(Vertex vertex, const ValueType &value) {
        values_[vertex] = value;
    }

    /**
     * @brief Get all vertex values
     * @return Map from vertex to value
     */
    const std::map<Vertex, ValueType> &get_values() const { return values_; }
};

/**
 * @brief Solution providing winning regions and strategies (RS capabilities)
 * @template GraphType The graph type (ParityGraph, MeanPayoffGraph)
 * @template StrategyType The strategy type (default: DeterministicStrategy)
 */
template <typename GraphType, typename StrategyType = DeterministicStrategy<GraphType>>
class RSSolution : public RSolution<GraphType>, public SSolution<GraphType, StrategyType> {
  public:
    RSSolution() = default;
    explicit RSSolution(bool solved, bool valid = true) {
        this->set_solved(solved);
        this->set_valid(valid);
    }
};

/**
 * @brief Solution providing winning regions and quantitative values (RQ capabilities)
 * @template GraphType The graph type (ParityGraph, MeanPayoffGraph)
 * @template ValueType The type of values associated with vertices (default: double)
 */
template <typename GraphType, typename ValueType = double>
class RQSolution : public RSolution<GraphType>, public QSolution<GraphType, ValueType> {
  public:
    RQSolution() = default;
    explicit RQSolution(bool solved, bool valid = true) {
        this->set_solved(solved);
        this->set_valid(valid);
    }
};

/**
 * @brief Complete solution providing all capabilities (RSQ capabilities)
 * @template GraphType The graph type (ParityGraph, MeanPayoffGraph)
 * @template StrategyType The strategy type (default: DeterministicStrategy)
 * @template ValueType The type of values associated with vertices (default: double)
 */
template <typename GraphType, typename StrategyType = DeterministicStrategy<GraphType>, typename ValueType = double>
class RSQSolution : public RSolution<GraphType>, public SSolution<GraphType, StrategyType>, public QSolution<GraphType, ValueType> {
  public:
    RSQSolution() = default;
    explicit RSQSolution(bool solved, bool valid = true) {
        this->set_solved(solved);
        this->set_valid(valid);
    }
};

// =============================================================================
// Solver Interface
// =============================================================================

/**
 * @brief Generic solver interface for game graphs
 * @template GraphType The graph type (ParityGraph, MeanPayoffGraph)
 * @template SolutionType The solution type returned by the solver
 */
template <typename GraphType, typename SolutionType>
class Solver {
  public:
    virtual ~Solver() = default;

    /**
     * @brief Solve the given game graph and return a solution
     * @param graph Game graph to solve
     * @return Solution of the specified type
     */
    virtual SolutionType solve(const GraphType &graph) = 0;

    /**
     * @brief Get solver name/description
     * @return String description of the solver
     */
    virtual std::string get_name() const = 0;
};

} // namespace solvers
} // namespace ggg
