# MSCA Solver

This directory contains the **MSCA (Mean-payoff Solver with Constraint Analysis)** solver implementation for mean-payoff games in the Game Graph Gym framework.

## Algorithm

The MSCA solver implements the Dorfman/Kaplan/Zwick algorithm from:

- **"Simple Stochastic Games with Few Random Vertices are Easy to Solve"** (ICALP 2019)
- Rectified implementation details from **arXiv:2310.04130**

### Key Features

- **Scaling-based approach**: Uses binary scaling with energy functions to achieve polynomial complexity
- **Energy manipulation**: Maintains energy values (`msrfun`) for each vertex and applies delta lifting
- **Complete solution**: Provides winning regions, strategies for Player 0, and quantitative values

### Algorithm Overview

1. **Initialization**: Set up vertex mappings, weights, and energy functions
2. **Energy Computation**: Apply recursive scaling when negative scaled weights are detected
3. **Delta Operations**: Compute optimal energy increases via `deltaP1()` and `deltaP2()` 
4. **Energy Updates**: Propagate changes through the graph using `updateEnergy()`
5. **Solution Extraction**: Determine winners based on final energy values vs. `nW/2` threshold

### Time Complexity

The algorithm achieves **strongly polynomial time** for mean-payoff games, which is a significant theoretical improvement over exponential algorithms.

## Usage

The solver integrates with the standard GGG CLI interface:

```bash
# Build the solver
cmake --build build --target msca_solver

# Run on a mean-payoff game
./build/solvers/meanpayoff/msca/msca_solver < game.dot

# With timing and CSV output
./build/solvers/meanpayoff/msca/msca_solver --csv --time-only < game.dot
```

## Implementation Notes

- Uses `boost::dynamic_bitset` for efficient set operations (`setL`, `setB`)
- Maintains detailed statistics for algorithmic analysis
- Handles edge cases like games with all-zero weights
- Player 1 strategies are not computed (as noted in original implementation)
- Memory-efficient vertex indexing for large graphs

## Statistics Output

The solver provides detailed performance metrics:
- Update operations and delta lifts
- Scaling operations and total arithmetic operations
- Delta iteration counts and effectiveness measures
- Maximum delta values for analysis
