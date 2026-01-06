# Buchi Game Solver

This directory contains a Buchi game solver implementation for the GameGraphGym (ggg) library.

## Algorithm Description

The solver implements a **fixpoint-based algorithm** for solving Buchi games. Buchi games are a special case of parity games where priorities are restricted to 0 or 1, and player 0 wins if they can force a play that visits priority 1 vertices infinitely often.

### Objective
- **Player 0 wins**: Can force infinite visits to priority 1 vertices (Buchi accepting vertices)
- **Player 1 wins**: Can eventually avoid all priority 1 vertices forever

### Algorithm Details

The algorithm computes the greatest fixpoint:
```
νX. μY. Attr₀(F ∩ X) ∪ Attr₁(Y)
```

Where:
- `F` = set of Buchi accepting vertices (priority 1)
- `Attr₀(S)` = attractor set for player 0 to set S
- `Attr₁(S)` = attractor set for player 1 to set S
- `X` = vertices from which player 0 wins
- `Y` = intermediate computation in the inner fixpoint

### Time Complexity
- **O(n × m)** where n is the number of vertices and m is the number of edges
- The algorithm terminates in at most n iterations of the outer fixpoint

### Implementation Features

1. **Input Validation**: Ensures all priorities are 0 or 1
2. **Strategy Computation**: Provides winning strategies for both players
3. **Robust Fixpoint**: Uses standard fixpoint iteration with proper termination
4. **Attractor Computation**: Efficient computation of attractor sets

## Usage

```cpp
#include "buchi_solver.hpp"
#include "libggg/graphs/parity_graph.hpp"

using namespace ggg::graphs;
using namespace ggg::solvers;

// Create a Buchi game (using ParityGraph with priorities 0/1)
ParityGraph graph;
auto v0 = addVertex(graph, "v0", 0, 0);  // Player 0, priority 0
auto v1 = addVertex(graph, "v1", 1, 1);  // Player 1, priority 1 (Buchi accepting)
addEdge(graph, v0, v1);
addEdge(graph, v1, v0);

// Solve the game
BuchiSolver solver;
auto solution = solver.solve(graph);

if (solution.solved) {
    // Check winning regions
    if (solution.isWonByPlayer0(v0)) {
        std::cout << "Player 0 wins from v0" << std::endl;
    }
    
    // Get strategy
    auto next = solution.getStrategy(v0);
}
```

## Files

- `buchi_solver.hpp` - Header file with class declaration
- `buchi_solver.cpp` - Implementation of the Buchi solver
- `test_buchi_solver.cpp` - Test cases demonstrating usage
- `CMakeLists.txt` - Build configuration

## Building

The solver is built as part of the main ggg project:

```bash
cd build
cmake .. -DBUILD_ALL_SOLVERS=ON
make buchi_solver
```

To run tests:
```bash
cd build/solvers/buchi
./test_buchi_solver
```

## References

1. Grädel, E., Thomas, W., & Wilke, T. (Eds.). (2002). *Automata logics, and infinite games: a guide to current research*. Springer.
2. Chatterjee, K., & Henzinger, T. A. (2012). A survey of stochastic ω-regular games. *Journal of Computer and System Sciences*, 78(2), 394-413.
