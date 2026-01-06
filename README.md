# Fully Polynomial Randomised Additive Approximation Scheme (FPRAAS) for Random Arena Games

This repository contains tools for generating, analysing, and solving three types of game arenas: **Reachability Games**, **Parity Games**, and **Energy Games**. Each game type can be generated randomly, solved exactly, and analysed using the FPRAAS (Fully Polynomial Randomized Approximation Scheme) sampling algorithm.

## Tools Used

- **Reachability and Parity Games**: We use the [Game Graph Gym (ggg)](https://github.com/gamegraphgym/ggg) tool to generate game arenas and solve games.
- **Energy Games**: We use the [egsolver](https://github.com/pazz/egsolver) tool to generate game arenas and solve games.

## Table of Contents

- [Installation](#installation)
- [Reachability Games](#reachability-games)
- [Parity Games](#parity-games)
- [Energy Games](#energy-games)
- [Results](#results)
- [Docker](#docker)

---

## Installation

This repository includes both **ggg** (Game Graph Gym) for reachability and parity games, and **egsolver** for energy games. You need to build/install them before using the scripts.

### Building ggg (Game Graph Gym)

ggg is included in the `ggg` directory. This repository uses ggg from git commit `5d31ac68e36cd781bfa00b9672ea0861872c45cf`. Build it as follows:

1. Navigate to the ggg directory:
   ```bash
   cd ggg
   ```

2. Build ggg:
   ```bash
    mkdir -p build
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_ALL_SOLVERS=ON -DBUILD_TOOLS=ON 
    cmake --build build -j$(nproc)
   ```

3. The solvers will be built in:
   - `ggg/build/solvers/parity/reachability/ggg_reachability`
   - `ggg/build/solvers/parity/priority_promotion/priority_promotion_solver`

4. Return to the repository root:
   ```bash
   cd ../..
   ```

### Installing egsolver

egsolver is a Python package included in the `egsolver` directory. This repository uses egsolver updated from the original repo commit `4f8c500f892c10073a8e3b52cf6700ddbf2deeda` to work with Python 3. Install it using pip:

1. Create a virtual environment (recommended):
   ```bash
   python3 -m venv venv
   ```

2. Activate the virtual environment:
   ```bash
   source venv/bin/activate
   ```

3. Navigate to the egsolver directory:
   ```bash
   cd egsolver
   ```

4. Install egsolver and its dependencies:
   ```bash
   pip install .
   ```

5. Verify installation:
   ```bash
   egsolver --version
   ```

6. Return to the repository root:
   ```bash
   cd ..
   ```

**Note:** Remember to activate the virtual environment (`source venv/bin/activate`) before running scripts that use egsolver.

**Note:** egsolver requires Python packages `networkx>=2.0` and `numpy`, which will be installed automatically with pip.

**Note:** Make sure both tools are properly built and installed before running the scripts in the `experiments` directory.

### Running Experiments

After installation, navigate to the experiments directory to run the scripts:

```bash
cd experiments
```



## Reachability Games

### Generating a Random Game Arena

Reachability games use the same generation script as parity games. To generate a random reachability game arena:

```bash
python generate_parity_games.py -o <output_directory> -n <number_of_games> -v <vertices> --max-priority <max_priority> --min-out-degree <min_degree> --max-out-degree <max_degree> -s <seed>
```

**Example:**
```bash
python generate_parity_games.py -o Reach -n 10 -v 5 --max-priority 1 --min-out-degree 1 --max-out-degree 5 -s 42 
```

This generates 10 reachability games with 5 vertices each, with priorities up to 1, and out-degrees between 1 and 5.

### Understanding the Game Arena (DOT File Format)

A game arena is represented as a DOT (Graphviz) file. The format is:

```
digraph ParityGame {
  v0 [name="v0", player=0, priority=0];
  v1 [name="v1", player=1, priority=1];
  v2 [name="v2", player=0, priority=2];
  ...
  v0 -> v1 [label="edge_0_1"];
  v1 -> v2 [label="edge_1_2"];
  ...
}
```

**Key Components:**
- **Vertices**: Each vertex `vN` has:
  - `name`: The vertex identifier
  - `player`: Either 0 (Player 0/MIN) or 1 (Player 1/MAX) - controls who moves from this vertex, which does not matter in our setting
  - `priority`: An integer priority value (for reachability games, this is used to determine winning conditions)
- **Edges**: Directed edges `vX -> vY` represent possible moves between vertices

In **reachability games**, the goal is to determine whether Player 0 (starting from vertex v0) can reach a target set of vertices (typically those with priority 0) regardless of Player 1's strategy.

### Solving the Exact Random Game

To compute the exact probability that Player 0 wins across all possible player assignments:

```bash
python compute-exact-probability-reach.py <input.dot> <output>
```

This script:
1. Analyses the game structure to check if early termination is possible
2. Enumerates all possible player assignments (2^n for n vertices)
3. For each assignment, runs the reachability solver
4. Aggregates the results to compute the exact winning probability
5. Saves results to `<input_name>_results.txt`

**Note:** The script will skip game arenas that have probability 0 or 1 based on structural analysis. Specifically, if v0 can only reach priority-0-only vertices, or if there is no priority-0-only lasso path from v0, the script terminates early without enumerating all player assignments, as the probability is deterministically 0 or 1.

**Batch processing:**
```bash
./run_exact_reach.sh
```

The solver used is: `ggg/build/solvers/parity/reachability/ggg_reachability` and each result is saved to `<input_name>_results.txt` as (wins / total_games).

### Running FPRAAS Sampling Algorithm

To estimate the winning probability using the FPRAAS sampling algorithm:

```bash
python fpraas-reach.py <input.dot> <output> <N>
```

Where `<N>` is the number of random samples to generate.

**Example:**
```bash
python fpraas-reach.py Reach/parity_game_1.dot Reach/tmp.dot 5
```

This script:
1. Performs graph analysis to check if sampling is needed
2. Generates N random player assignments
3. For each assignment, solves the game and records the result
4. Computes the estimated probability as (wins / total_samples)
5. Saves progress to `<input_name>_sampled_results.txt` (logs every 1000 samples)

**Progress Reporting:**
- **Console output**: Every 1000 samples, the script prints progress to the console in the format:
  ```
  Solved <games_solved>/<N> | Agg: <wins>/<games_solved> | Time: <total_time_ms> ms
  ```
- **Log file**: Every 1000 samples, a summary line is written to the results file:
  ```
  <wins>/<samples>; <total_time_ms>
  ```
- **Final summary**: Upon completion, the script prints:
  - Total samples solved
  - Total aggregated value (wins)
  - Total solver time
  - Average time per sample

**Batch processing:**
```bash
./run_fpraas_reach.sh
```

The output format in the results file is: `<wins>/<samples>; <total_time_ms>`

---

## Parity Games

### Generating a Random Game Arena

To generate random parity game arenas:

```bash
python generate_parity_games.py -o <output_directory> -n <number_of_games> -v <vertices> --max-priority <max_priority> --min-out-degree <min_degree> --max-out-degree <max_degree> -s <seed>
```

**Example:**
```bash
python generate_parity_games.py -o Parity -n 10 -v 5 --max-priority 5 --min-out-degree 1 --max-out-degree 5 -s 1234 
```

This generates 10 parity games with 5 vertices each, priorities up to 5, and out-degrees between 1 and 5.

### Understanding the Game Arena (DOT File Format)

Parity games use the same DOT format as reachability games:

```
digraph ParityGame {
  v0 [name="v0", player=0, priority=3];
  v1 [name="v1", player=1, priority=5];
  ...
  v0 -> v1 [label="edge_0_1"];
  ...
}
```

**Key Components:**
- **Vertices**: Same structure as reachability games
  - `player`: Controls who moves (0 or 1)
  - `priority`: Integer priority value (critical for parity winning condition)
- **Edges**: Directed edges representing moves

In **parity games**, the winner is determined by the highest priority that appears infinitely often in a play. Player 0 wins if the highest priority seen infinitely often is even; Player 1 wins if it's odd.

### Solving the Exact Random Game

To compute the exact probability that Player 0 wins:

```bash
python compute-exact-probability-parity.py <input.dot> <output.dot>
```

This script:
1. Checks for lasso paths with even/odd highest priorities
2. Enumerates all 2^n player assignments
3. For each assignment, runs the parity solver
4. Aggregates results to compute exact probability
5. Saves to `<input_name>_results.txt`

**Note:** The script will skip game arenas that have probability 0 or 1 based on structural analysis. Specifically, if v0 cannot reach a simple cycle with highest priority even, or if all simple cycles reachable from v0 have highest priority even, the script terminates early without enumerating all player assignments, as the probability is deterministically 0 or 1.

**Batch processing:**
```bash
./run_exact_parity.sh
```

The solver used is: `ggg/build/solvers/parity/priority_promotion/priority_promotion_solver`

### Running FPRAAS Sampling Algorithm

To estimate the winning probability using FPRAAS:

```bash
python fpraas-parity.py <input.dot> <output.dot> <N>
```

**Example:**
```bash
python fpraas-parity.py Parity/parity_game_1.dot Parity/tmp.dot 5
```

This script:
1. Analyses the game for even/odd cycle structures
2. Samples N random player assignments
3. Solves each sampled game
4. Estimates probability from sample results
5. Saves progress to `<input_name>_sampled_results.txt`

**Progress Reporting:**
- **Console output**: Every 1000 samples, the script prints progress to the console in the format:
  ```
  Solved <games_solved>/<N> | Agg: <wins>/<games_solved> | Time: <total_time_ms> ms
  ```
- **Log file**: Every 1000 samples, a summary line is written to the results file:
  ```
  <wins>/<samples>; <total_time_ms>
  ```
- **Final summary**: Upon completion, the script prints:
  - Total samples solved
  - Total aggregated value (wins)
  - Total solver time
  - Average time per sample

**Batch processing:**
```bash
./run_fpraas_parity.sh
```

---

## Energy Games

### Generating a Random Game Arena

Energy games should be generated using `egsolver`. Energy games are represented in JSON format, which includes:
- `nodes`: Array of vertices with `id` and `owner` (player)
- `edges`: Array of edges with `source`, `target`, and `effect` (weight)

To generate energy games, use the `egsolver` tool with appropriate parameters. Consult the `egsolver` documentation for specific generation commands and options.
**Example:**
```bash
mkdir Energy
egsolver generate 5 0.5 0.5 5 Energy/1.json
```

This generates an energy game with 5 vertices each, edge- and owner probability 1/2 and maximal absolute effect 5.

### Understanding the Game Arena (JSON File Format)

Energy games generated by `egsolver` use JSON format:

```json
{
  "nodes": [
    {"id": 0, "owner": 0},
    {"id": 1, "owner": 1},
    ...
  ],
  "edges": [
    {"source": 0, "target": 1, "effect": 2},
    {"source": 1, "target": 0, "effect": -1},
    ...
  ]
}
```

**Key Components:**
- **Nodes**: Array of vertices, each with:
  - `id`: The vertex identifier (integer)
  - `owner`: 0 (Player 0/MIN) or 1 (Player 1/MAX) - controls who moves from this vertex
- **Edges**: Array of directed edges, each with:
  - `source`: Source vertex ID
  - `target`: Target vertex ID
  - `effect`: Energy change when traversing this edge (can be negative)

In **energy games**, Player 0 (MAX) wins if they can maintain non-negative energy forever (starting from a given initial credit). Player 1 (MIN) wins if they can force the energy to drop below zero.

### Solving the Exact Random Game

To compute the exact probability that Player 0 wins for JSON format energy games:

```bash
python compute-exact-probability-energy.py <input.json> <output.json>
```

These scripts:
1. Check for nonnegative-energy lasso paths and negative-energy paths
2. Enumerate all player assignments
3. For each assignment, run the energy solver
4. Compute exact winning probability
5. Save results to `<input_name>_energy_results.txt`

**Note:** The scripts will skip game arenas that have probability 0 or 1 based on structural analysis. Specifically, if v0 cannot maintain nonnegative energy (no nonnegative-energy lasso exists), or if there exists a path where energy drops below zero, the script terminates early without enumerating all player assignments, as the probability is deterministically 0 or 1.


The solver used is: `egsolver solve <file.json>`

### Running FPRAAS Sampling Algorithm

To estimate the winning probability using FPRAAS:

```bash
python fpraas-energy.py <input.json> --samples <N> --workers <num_workers> --tmpdir <temp_dir>
```

**Example:**
```bash
python fpraas-energy.py Energy/1.json --samples 5 --workers 2 --tmpdir tmp_solvers
```

This script:
1. Analyses the game structure for energy properties
2. Samples N random player assignments (in parallel)
3. Solves each sampled game using the energy solver
4. Estimates probability from aggregated results
5. Saves progress to `<input_name>_energy_results_sampled.txt`

**Progress Reporting:**
- **Console output**: Every 1000 samples, the script prints progress to the console in the format:
  ```
  <wins>/<samples>; <total_time_ms>
  ```
- **Log file**: Every 1000 samples, the same summary line is written to the results file:
  ```
  <wins>/<samples>; <total_time_ms>
  ```
- **Final summary**: Upon completion, the script prints:
  - Total aggregated value (wins) and total samples
  - Total solver time (wall-clock time)

**Key Features:**
- Parallel processing with configurable worker count
- Progress logging every 1000 samples
- Automatic cleanup of temporary files

---

## Results

For each objective, we generated 10 random game graphs with 20 nodes and maximum outdegree of 20; parity objectives used up to 6 priorities, and energy objectives used an initial credit of 0 with weights in {-10, ... , 10}.

### Game Arena Folders

The experiments use the following folders to store random game arenas:
- `Reach/` - Reachability game arenas (DOT format): generated with `python generate_parity_games.py -o Reach -n 20 -v 20 --max-priority 1 --min-out-degree 1 --max-out-degree 20 -s 4242`
- `Parity/` - Parity game arenas (DOT format): generated with `python generate_parity_games.py -o Parity -n 20 -v 20 --max-priority 5 --min-out-degree 1 --max-out-degree 20 -s 1234`
- `Energy/` - Energy game arenas (JSON format): generated with `egsolver generate 20 0.5 0.5 10 Energy/1.json`

### Results Folders

The `plot.py` script reads result files from the following directories:
- `results/reach-sample/` - Reachability game sampling results  
- `results/parity-sample/` - Parity game sampling results
- `results/energy-sample/` - Energy game sampling results

Each `*.txt` file in these directories follows this format:
- **First line**: The exact probability (computed from exhaustive enumeration, e.g., `557056/1048576`)
- **Following lines**: FPRAAS sampling results in format `wins/samples; time` (e.g., `100/1000; 1234.5`)

The plot compares the FPRAAS sampling estimates (from the following lines) against the exact probability (from the first line) to visualize convergence.

### Plotting Results

The `plot.py` script visualizes FPRAAS sampling results by plotting the convergence of estimated probabilities.

### Running plot.py

1. Ensure you are in the virtual environment:
   ```bash
   source venv/bin/activate
   ```

2. Install required dependencies:
   ```bash
   pip install numpy matplotlib pandas
   ```

3. Ensure result files are in the appropriate `results/*-sample/` directories

4. Edit `plot.py` to configure the plotting options (folders, parameters, save path)

5. Run the script:
   ```bash
   python plot.py
   ```

The script will generate plots comparing the convergence of sampling estimates across different game types.

---

## General Notes

### Solver Paths

The scripts use hardcoded solver paths. Make sure these exist or update the paths in the Python scripts:
- Reachability: `ggg/build/solvers/parity/reachability/ggg_reachability`
- Parity: `ggg/build/solvers/parity/priority_promotion/priority_promotion_solver`
- Energy: `egsolver` (must be in PATH)

### Output Files

- **Exact results**: `<game_name>_results.txt` or `<game_name>_energy_results.txt`
- **Sampled results**: `<game_name>_sampled_results.txt` or `<game_name>_energy_results_sampled.txt`

Results files contain aggregated statistics showing wins/total and timing information.

### Early Termination

All `compute-exact-probability-xxx.py` and `fpraas-xxx.py` scripts include early termination checks to skip game arenas that have probability 0 or 1 based on graph structure analysis. This avoids unnecessary computation by detecting deterministic outcomes before enumerating all player assignments. The specific conditions vary by game type:

- **Reachability games**: Skip if v0 can only reach target vertices, or if v0 cannot reach a target state
- **Parity games**: Skip if v0 cannot reach cycles with even highest priority, or if all cycles have even highest priority
- **Energy games**: Skip if v0 cannot maintain nonnegative energy, or if negative-energy paths do not exist

---

## Docker

A Dockerfile is provided to run the experiments in a containerized environment.

### Loading a Pre-built Docker Image

If a pre-built Docker image (`random-arena-games.tar`) is available, you can load it instead of building from scratch:

```bash
docker load -i random-arena-games.tar
```

This will load the image with the tag `random-arena-games:latest`. After loading, you can proceed to [Running the Docker Container](#running-the-docker-container).

### Building the Docker Image

**Note:** Make sure Docker Engine is running before building the image.

From the repository root directory, build the Docker image:

```bash
docker build -t random-arena-games .
```

This will:
- Install all system dependencies (Python, cmake, build tools)
- Build ggg solvers
- Set up Python virtual environment
- Install egsolver and plotting dependencies

### Running the Docker Container

Start an interactive container:

```bash
docker run -it random-arena-games
```

This will drop you into a bash shell inside the container. Once the Docker container is running:
- The virtual environment is already activated
- The ggg tools are already installed and built
- You'll be in the `/workspace/experiments` directory

### Saving Plots

After running `plot.py` inside the container, to copy the plot file to your local machine:

1. Get the container ID:
   ```bash
   docker ps -a
   ```

2. Copy the plot file from the container:
   ```bash
   docker cp <container_id>:/workspace/experiments/plot.png .
   ```

This will copy `plot.png` from the container to your current directory.

### Exiting the Docker Container

To exit the container while keeping it running in the background, press `Ctrl+P` followed by `Ctrl+Q`.

To stop and exit the container:

```bash
exit
```

Or from another terminal, you can stop a running container:

```bash
docker ps  # Find the container ID
docker stop <container_id>
```

