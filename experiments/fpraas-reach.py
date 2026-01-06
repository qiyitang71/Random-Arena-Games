import re
import argparse
import random
import os
import subprocess
import time
from collections import deque

# ------------------------------------------------
# Parse DOT graph
# ------------------------------------------------
def read_graph(dot_file):
    """
    Parse the DOT file and return:
      - vertices: dict of {vertex_name: priority}
      - edges: adjacency list {vertex: [neighbors]}
    """
    vertex_pattern = re.compile(
        r'^(?P<vertex>v\d+)\s+\[name="[^"]+",\s+player=(?P<player>\d+),\s+priority=(?P<priority>\d+)\];'
    )
    edge_pattern = re.compile(
        r'^(?P<source>v\d+)\s*->\s*(?P<target>v\d+)'
    )

    vertices = {}
    edges = {}

    with open(dot_file, "r") as f:
        for line in f:
            line = line.strip()
            v_match = vertex_pattern.match(line)
            e_match = edge_pattern.match(line)
            if v_match:
                v = v_match.group("vertex")
                priority = int(v_match.group("priority"))
                vertices[v] = priority
                if v not in edges:
                    edges[v] = []
            elif e_match:
                src = e_match.group("source")
                tgt = e_match.group("target")
                edges.setdefault(src, []).append(tgt)

    return vertices, edges


# ------------------------------------------------
# Graph checks
# ------------------------------------------------
def is_priority0_only_region(start_vertex, vertices, edges):
    """
    Check if the start_vertex has priority 0 and all vertices reachable from it
    also have priority 0.
    """
    if vertices.get(start_vertex, -1) != 0:
        return False

    visited = set()
    queue = deque([start_vertex])

    while queue:
        current = queue.popleft()
        if current in visited:
            continue
        visited.add(current)

        if vertices.get(current, -1) != 0:
            return False

        for neighbor in edges.get(current, []):
            if neighbor not in visited:
                queue.append(neighbor)

    return True


def v0_reaches_only_priority0(vertices, edges):
    """Check whether v0 can only reach vertices in priority-0-only regions."""
    if "v0" not in vertices:
        print("Warning: v0 not found in the graph.")
        return False

    visited = set()
    queue = deque(["v0"])

    while queue:
        current = queue.popleft()
        if current in visited:
            continue
        visited.add(current)

        if vertices.get(current, -1) != 0:
            return False

        for neighbor in edges.get(current, []):
            queue.append(neighbor)

    return True


def has_full_priority0_lasso(vertices, edges):
    """
    Check if there is a lasso-like path starting from v0 such that
    every vertex along the path and in the cycle has priority 0.
    """
    if "v0" not in vertices or vertices["v0"] != 0:
        return False

    stack = [("v0", [])]  # (current_vertex, path_from_v0)

    while stack:
        current, path = stack.pop()

        if vertices.get(current, -1) != 0:
            continue

        # Cycle detection
        if current in path:
            # All vertices along path have already been checked for priority=0
            return True

        new_path = path + [current]
        for neighbor in edges.get(current, []):
            stack.append((neighbor, new_path))

    return False


# ------------------------------------------------
# Player replacement
# ------------------------------------------------
def replace_players_in_dot(dot_file, output_file, bit_string):
    """Replace player assignments in DOT file according to bit_string."""
    vertex_pattern = re.compile(
        r'^(?P<vertex>v\d+)\s+\[name="[^"]+",\s+player=(?P<player>\d+),\s+priority=(?P<priority>\d+)\];'
    )

    with open(dot_file, "r") as f:
        lines = f.readlines()

    updated_lines = []
    for line in lines:
        match = vertex_pattern.match(line.strip())
        if match:
            vertex_name = match.group("vertex")
            vertex_index = int(vertex_name[1:])
            new_player = bit_string[vertex_index]
            new_line = re.sub(r'player=\d+', f'player={new_player}', line)
            updated_lines.append(new_line)
        else:
            updated_lines.append(line)

    with open(output_file, "w") as f:
        f.writelines(updated_lines)


# ------------------------------------------------
# Solver output parsing
# ------------------------------------------------
def parse_solver_csv(output):
    """
    Parse solver output in CSV mode.
    Look for the line starting with 'v0', e.g. 'v0,1,1,v0,0.012'.
    If the third value is 1, return 1; otherwise 0.
    """
    for line in output.splitlines():
        if line.startswith("v0"):
            parts = line.split(",")
            if len(parts) >= 3:
                return 1 if parts[2].strip() == "0" else 0
    return 0


# ------------------------------------------------
# Main
# ------------------------------------------------
def main():
    parser = argparse.ArgumentParser(
        description="Run solver on random player assignments."
    )
    parser.add_argument("input_file", help="Path to the input DOT file.")
    parser.add_argument("output_file", help="Path to save the modified DOT file.")
    parser.add_argument("N", type=int, help="Number of random samples to generate.")
    args = parser.parse_args()

    # --- Graph Analysis ---
    vertices, edges = read_graph(args.input_file)
    n = len(vertices)

    v0_priority0_only = v0_reaches_only_priority0(vertices, edges)
    v0_lasso = has_full_priority0_lasso(vertices, edges)

    print(f"v0 can only reach priority-0-only vertices: {v0_priority0_only}")
    print(f"v0 has a priority-0-only lasso path: {v0_lasso}")

    # --- Early Termination ---
    if v0_priority0_only or not v0_lasso:
        print("Early termination: no need to sample.")
        return

    print(f"Detected {n} vertices. Sampling {args.N} random player assignments...")

    # Prepare log
    log_file = f"{os.path.splitext(args.input_file)[0]}_sampled_results.txt"
    total_aggregated = 0
    total_time_ms = 0.0
    games_solved = 0

    with open(log_file, "w") as logf:
        for i in range(args.N):
            bit_string = "".join(random.choice("01") for _ in range(n))
            replace_players_in_dot(args.input_file, args.output_file, bit_string)

            start = time.perf_counter()
            try:
                solver_output = subprocess.check_output([
                    "../ggg/build/solvers/parity/reachability/ggg_reachability",
                    "-i", args.output_file,
                    "--csv"
                ], text=True)
            except subprocess.CalledProcessError as e:
                solver_output = f"Solver error: {e}"
            end = time.perf_counter()

            elapsed_ms = (end - start) * 1000
            total_time_ms += elapsed_ms
            agg_value = parse_solver_csv(solver_output)
            total_aggregated += agg_value
            games_solved += 1

            # logf.write(f"Sample {i+1}/{args.N} bitstring: {bit_string}\n")
            # logf.write(solver_output.strip() + "\n")
            # logf.write(f"Python measured time: {elapsed_ms:.3f} ms\n")
            # logf.write(f"Winner aggregated value: {agg_value}\n")
            # logf.write("-" * 40 + "\n")

            # Console progress (print only every 1000 games)
            if games_solved % 1000 == 0:
                logf.write(f"{total_aggregated}/{games_solved}; {total_time_ms:.3f}\n")
                logf.flush()  # <-- force write to disk
                print(f"Solved {games_solved}/{args.N} | "
                    f"Agg: {total_aggregated}/{games_solved} | Time: {total_time_ms:.3f} ms")

    print(f"\nAll results saved to {log_file}")
    print(f"Total samples solved: {games_solved}/{args.N}")
    print(f"Total aggregated value: {total_aggregated}")
    print(f"Total Python-measured solver time: {total_time_ms:.3f} ms")
    print(f"Average time per sample: {total_time_ms / games_solved:.3f} ms")


if __name__ == "__main__":
    main()