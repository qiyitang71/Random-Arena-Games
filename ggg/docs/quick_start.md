# Quick Start Guide {#quick_start}

## Prerequisites

Ensure you have the required dependencies installed:
The following lists packages as in Ubuntu/Debian. For other platforms (macOS, Fedora/RHEL, Windows), see the [detailed build instructions](building.md#platform-specific-setup).

```bash
sudo apt-get install -y build-essential cmake \
    libboost-graph-dev libboost-program-options-dev \
    libboost-filesystem-dev libboost-system-dev libboost-test-dev
```


## Building Tools and Solvers

```bash
# Clone the repository
git clone https://github.com/pazz/ggg.git
cd ggg

# Configure build with all components
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_ALL_SOLVERS=ON \
    -DBUILD_TOOLS=ON

# Build the project
cmake --build build -j$(nproc)
```

## Running Your First Example

After building, try running the basic usage example:

```bash
# Generate some test games
./build/bin/ggg_generate_parity_games -o test_games -n 5 -v 20

# Run a solver on the generated games
./build/solvers/parity/recursive/ggg_recursive -i test_games/game_0.dot
```

## Using the API in Your Code

Here's a minimal example of using Game Graph Gym in your own project:

```cpp
#include <libggg/libggg.hpp>
#include "solvers/parity/recursive/recursive_solver.hpp"  // get hold of some solver

int main() {
    // Load a Parity Graph from DOT file
    auto game = ggg::graphs::parse_parity_graph_from_file("game.dot");
    
    // Solve using a solver that provides regions and strategies (RS solution)
    auto solver = std::make_unique<ggg::solvers::RecursiveParitySolver>();
    auto solution = solver->solve(game);
    
    // Check results
    if (solution.is_solved()) {
        std::cout << "Game solved! Vertex v0 won by player: " 
                  << solution.get_winning_player(v0) << std::endl;
    }
}
```

## Next Steps

- Read the [detailed build instructions](building.md)
- Explore the API reference documentation