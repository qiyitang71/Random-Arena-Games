# API Guide {#api-guide}

This guide covers the core APIs for extending Game Graph Gym with new game types and solvers.

## Adding New Game Types

Game Graph Gym uses X-macros to automatically generate graph types and utility functions. To add a new game type, you just need to define the vertex, edge, and graph properties and then call the `DEFINE_GAME_GRAPH` macro as follows.

```cpp
// my_graph.hpp
#include <libggg/graphs/graph_utilities.hpp>

// Define vertex properties
#define VERTEX_FIELDS(F) \
    F(std::string, name) \
    F(int, player) \
    F(int, custom_property)

// Define edge properties
#define EDGE_FIELDS(F) \
    F(std::string, label)

// Define graph properties (if any)
#define GRAPH_FIELDS(F) \
    F(std::string, title)

// Generate the complete graph type with utilities
DEFINE_GAME_GRAPH(MyGraph, VERTEX_FIELDS, EDGE_FIELDS, GRAPH_FIELDS)

// Optionally add utilities specific to your graph type
inline bool is_valid(const MyGraph &graph) {
    return true;
}
```

This will generate

- `MyGraphGraph` - Boost adjacency list with bundled properties
- `MyGraphVertex`, `MyGraphEdge` - Type aliases for descriptors
- `add_vertex()`, `add_edge()` - Utility functions with named parameters
- `parse_MyGraph_graph()` - DOT format parsing functions
- `write_MyGraph_graph()` - DOT format writing functions

To use the generated types and functions:

```cpp
#include my_graph.hpp

// Create graph and add vertices with named parameters
MyGraphGraph graph;
const auto v1 = add_vertex(graph, "vertex1", 0, 42);
const auto v2 = add_vertex(graph, "vertex2", 1, 24);

// Add edges with properties
add_edge(graph, v1, v2, "edge_label");

// Parse from file or stream
auto graph_ptr = parse_MyGraph_graph("input.dot");
auto graph_ptr2 = parse_MyGraph_graph(std::cin);

// Write to file or stream
write_MyGraph_graph(*graph_ptr, "output.dot");
write_MyGraph_graph(*graph_ptr, std::cout);
```

## Adding New Solvers

### Implementing a Solver

To implement a new solver, inherit from the unified `Solver` interface with the appropriate [solution type](#solution-types) and implement the `solve()` method. For example, implement a new solver for parity games, that operates on `ParityGraph` and produces solutions which encode winning regions and positional strategies (`RSSolution<ParityGraph>`) as follows.

```cpp
#include "libggg/graphs/parity_graph.hpp"
#include "libggg/solvers/solver.hpp"

// Example solver providing winning regions and strategies
class YourSolver : public ggg::solvers::Solver<ParityGraph, ggg::solvers::RSSolution<ParityGraph>> {
public:
    ggg::solvers::RSSolution<ParityGraph> solve(const ParityGraph& game) override {
        ggg::solvers::RSSolution<ParityGraph> solution(true); // Mark as solved

        const auto [vertices_begin, vertices_end] = boost::vertices(game);

        for (auto it = vertices_begin; it != vertices_end; ++it) {
            const auto vertex = *it;
            const auto winner = computeWinner(game, vertex);
            solution.setWinningPlayer(vertex, winner);

            // Set strategy if applicable
            const auto strategy = computeStrategy(game, vertex);
            if (strategy != vertex) { // Valid strategy
                solution.setStrategy(vertex, strategy);
            }
        }

        return solution;
    }

    std::string getName() const override {
        return "Your Custom Solver Name";
    }
};
```

You can use a macro to turn this solver class into a command-line executable:

```C++
GGG_GAME_SOLVER_MAIN(GraphType, parse_GraphType_graph, YourSolver)
```

See the implementations of existing solvers under `solvers/` as examples.

## Solution Types

We define different solution capabilities via C++ concepts and inheritance.
Solution types are named `XSolution`, where the prefix `X` indicates what is encoded:
initial state only (`X=I`), winning regions (`R`), a winning strategy (`S`), and quantitative (vertex) values (`Q`).
For instance, `RSSolution<graphs::ParityGraph>`C++ is a type of solution operating on `ParityGraph`s and which can compute a winning `R`egion as well as synthesize a `S`trategy. In more details,

### `ISolution<GraphType>`

The most basic solution type on `<GraphType>` graphs. Can only veryfy status of initial state.
Solutions of this kind implement:

  - `is_solved()` - Check if solution is complete
  - `is_valid()` - Check if solution is valid

### `RSolution<GraphType>`
Contains info about winning **R**egions.
In addition to the inherited methods of `ISolution`, solutions of this kind implement:

  - `is_won_by_player0(vertex)` - Check if vertex is won by player 0
  - `is_won_by_player1(vertex)` - Check if vertex is won by player 1
  - `get_winning_player(vertex)` - Get winning player (0, 1, or -1)
  - `set_winning_player(vertex, player)` - Set winning player for vertex

### `SSolution<GraphType>`
Contains info about a winning **S**trategy
In addition to the inherited methods of `ISolution`, solutions of this kind implement:

  - `get_strategy(vertex)` - Get strategic choice for vertex
  - `has_strategy(vertex)` - Check if vertex has strategy defined
  - `set_strategy(vertex, strategy)` - Set strategy for vertex
  There is actually a second generic parameter specifying the type of strategy contained (mixing, deterministic, positional, finite-memory etc). The default is `DeterministicStrategy`, representing deterministic and positional strategies (i.e. mappings from vertices to vertices).

### `QSolution<GraphType, ValueType>`
Contains info about **Q**ualitative (vertex) values of type `ValueType`.\
  In addition to the inherited methods of `ISolution`, solutions of this kind implement:

  - `get_value(vertex)` - Get numerical value for vertex
  - `has_value(vertex)` - Check if vertex has value defined
  - `set_value(vertex, value)` - Set value for vertex

### Combined Solution Types

- **`RSSolution<GraphType>`**: Regions + Strategies (inherits R and S methods)
- **`RQSolution<GraphType, ValueType>`**: Regions + Quantitative values (inherits R and Q methods)
- **`RSQSolution<GraphType, ValueType>`**: All capabilities (inherits R, S, and Q methods)

Choose the appropriate solution type based on what your solver provides.
