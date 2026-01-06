# Game Graph Gym <img src="https://github.com/pazz/ggg/raw/main/.github/logo.png" align="right" height="100" />

[![Build and Test](https://github.com/pazz/ggg/actions/workflows/ci.yml/badge.svg)](https://github.com/pazz/ggg/actions/workflows/ci.yml)
[![Documentation](https://github.com/pazz/ggg/actions/workflows/docs.yml/badge.svg)](https://github.com/pazz/ggg/actions/workflows/docs.yml)
[![License: Apache 2.0](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)

A C++ framework for easy implementing and benchmarking solvers for [games on graphs][GOG-book].
It comes in the form of

- a lightweight C++ library, built on top of the [Boost Graph Library][BGL] that defines data structures for various game graphs (plus utilities like parser/formatter/optimizations) and solvers, and generic algorithms. 
- a set of command-line tools for generating random benchmark corpora, filtering/optimising game graphs graphs, running benchmarks.

The library is independent from benchmarking tools, which are designed with inter-operability in mind. For instance, one can compare the performance of any game solver implementation as long as executable has a compatible I/O interface.  
This project also ships with implementations of several standard solvers, available through the API and stand-alone executables for comparisons and direct use.


See the documentation at [docs/](docs/) for detailed build instructions and development setup.

## Quick Start

You will need cmake and some boost libraries:
*Graph* as well as *Filesystem*, *System*, *ProgramOptions* (only for tools and solvers), and *Test* (only for unit testing).  
Get all you need and more via 
`apt-get install -y build-essential cmake libboost-all-dev` (Debian/Ubuntu) or `brew install cmake boost` (MacOS). Then,

```bash
# Clone the repository
git clone https://github.com/pazz/ggg.git && cd ggg

# Configure and build with all bells and whistles included
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_ALL_SOLVERS=ON -DBUILD_TOOLS=ON
cmake --build build -j$(nproc)
```

Done! You can now use the API in your own code or run the included solvers and benchmark tools.


## CLI Tools

There are command-line tools for benchmarking as well as executables for various game solvers.
For instance, benchmark parity game solvers like so:


```bash
# Generate 20 parity games with 100 vertices each and store them in directory `./games/`
./build/bin/ggg_generate_parity_games -o games -n 20 -v 100

# List available solvers for `parity` games within the solver path `build/solvers/` (add any compatible binaries there)
./build/bin/ggg_list_solvers -g parity -p build/solvers

# benchmark all solvers for `parity` games within path `build/solvers` on all games in `games/` and output the results in CSV format
./build/bin/ggg_benchmark_solvers -g parity -p build/solvers -d games/ --csv
```

The sources for benchmarking tools are under [`tools/`](tools/);
those for game solvers (executables) are under [`solvers/`](solvers/).


### Universal CLI Interface for Solvers

All solver binaries provide a standardized command-line interface:
- `--help`: Show help message
- `--csv`: Output results in CSV format
- `--time-only`: Only output timing information
- `--solver-name`: Display solver name
- `--input/-i`: Input file (default: stdin)
- `--output/-o`: Output file (default: stdout)

These binaries live in `SOLVER_PATH` which is structured as `game_type/solver_name/solver_name`.
For instance, after build as above, `build/solvers` will contain the executable for the recursive parity game solver as `parity/recursive/recursive`.

### External Solver Support

The framework supports external solvers (written in any language) by implementing the same CLI interface. External binaries placed in `SOLVER_PATH/gametype/` are automatically discovered and benchmarked alongside native solvers.


## Supported Game Types and their Representations

So far, supported are (two-player, zero-sum, non-stochastic) games with parity, mean-payoff, buechi, and (generalised) reachability conditions.  
Game graphs can be imported from and exported to [Graphviz DOT](https://graphviz.org/doc/info/lang.html) format with custom attributes for vertices and (directed) edges:

- **All Game Graph Types** admit properties `name` (string), `player` (int) on vertices and `label` (string) on edges.
- **Parity game graphs** add a property `priority` (int) on edges;
- **Mean-Payoff game graphs** add a property `weight` (int) on vertices.


## Documentation

The complete documentation is built with [MkDocs Material](https://squidfunk.github.io/mkdocs-material/). From within the project root:

```bash
pip install mkdocs-material
mkdocs serve  # Development server at localhost:8000
mkdocs build  # Static site in site/ directory
```


[GOG-book]: https://arxiv.org/abs/2305.10546
[BGL]: https://www.boost.org/doc/libs/release/libs/graph/
