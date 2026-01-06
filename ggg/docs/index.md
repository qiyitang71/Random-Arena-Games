# Game Graph Gym

Welcome to the Game Graph Gym documentation! This framework provides a comprehensive C++20 library for implementing and benchmarking solvers for games on graphs.

## Overview

**Game Graph Gym (GGG)** is a modern C++20 framework designed for implementing and benchmarking game-theoretic solvers. The architecture consists of:

- **Core Library** (`libggg`): Built on Boost Graph Library, provides graph types (Parity, Mean-Payoff) with standardized vertex/edge properties
- **Solver Framework**: Template-based with pluggable solution types (regions, strategies, values) using C++20 concepts
- **CLI Tools**: Standardized interface for game generation, solver listing, and benchmarking
- **Modular Solvers**: Each game type has subdirectories with independent solvers

## Getting Started

- [Quick Start Guide](quick_start.md) - Get up and running quickly
- [Building the Library](building.md) - Detailed build instructions
- [File Formats](file_formats.md) - Supported input/output formats

## Development

- [Developer Guide](developing.md) - Setup development environment and contribution guidelines

## Supported Game Types

Game Graph Gym supports several types of two-player, zero-sum, non-stochastic games:

- **Parity Games**: Games with parity winning conditions
- **Mean-Payoff Games**: Games with mean-payoff objectives  
- **Büchi Games**: Games with Büchi acceptance conditions
- **Reachability Games**: Games with reachability objectives

## File Formats

All game types support import/export via Graphviz DOT format with custom attributes. See the [File Formats](file_formats.md) guide for detailed specifications.

## Command-Line Tools

The framework includes several command-line utilities:

- `ggg_generate_*_games`: Generate random game instances
- `ggg_benchmark_solvers`: Benchmark solver performance
- `ggg_list_solvers`: Discover available solvers
- Individual solver executables with standardized interfaces

## Architecture

The framework uses modern C++20 patterns:

### Solution Capability System
Solutions are composed using capabilities:
- **R** = regions
- **S** = strategies  
- **Q** = quantitative values
- **I** = initial state

Use `RSolution<GraphType>`, `RSSolution<GraphType>`, etc.

### Solver Interface
All solvers inherit from `Solver<GraphType, SolutionType>` and implement:
- `solve()` - Main solving logic
- `getName()` - Solver identification

### Graph Types
- `ParityGraph` / `MPGraph` with bundled properties
- Access via `graph[vertex].property`
- Boost Graph Library integration

## Contributing

See the [Developer Guide](developing.md) for information on setting up a development environment and contributing to the project.
