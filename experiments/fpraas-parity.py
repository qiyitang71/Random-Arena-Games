import re
import argparse
import random
import os
import subprocess
import time
from collections import deque

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

def find_lasso_highest_priority(vertices, edges, parity=0):
    """
    Check whether there is a lasso path starting from v0 such that
    the cycle part has highest priority with the given parity.
    parity = 0 for even, 1 for odd.
    Returns True if such a lasso exists.
    """
    stack = [("v0", [])]  # (current_vertex, path_so_far)

    while stack:
        current, path = stack.pop()

        if current in path:
            # Found a cycle: cycle = path from first occurrence to end
            cycle_start = path.index(current)
            cycle = path[cycle_start:] + [current]

            # Highest priority in the cycle
            max_priority = max(vertices[v] for v in cycle)
            if max_priority % 2 == parity:
                return True
            continue

        new_path = path + [current]
        for neighbor in edges.get(current, []):
            stack.append((neighbor, new_path))

    return False


# ----------------------------
# Helper functions for convenience
# ----------------------------
def v0_lasso_even(vertices, edges):
    """Check if there exists a lasso reachable from v0 with cycle's highest priority even."""
    return find_lasso_highest_priority(vertices, edges, parity=0)

def v0_lasso_odd(vertices, edges):
    """Check if there exists a lasso reachable from v0 with cycle's highest priority odd."""
    return find_lasso_highest_priority(vertices, edges, parity=1)

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

def parse_solver_csv(output):
    """
    Parse solver output in CSV mode.
    We look for the line starting with 'v0', e.g. 'v0,1,1,v0,0.012'.
    If the third value is 1, we return the fifth value as float; otherwise 0.
    """
    for line in output.splitlines():
        if line.startswith("v0"):
            parts = line.split(",")
            if len(parts) >= 5:
                third = parts[2].strip()
                # value = float(parts[4].strip())
                return 1 if third == "0" else 0
    return 0

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

    v0_has_even_cycle = v0_lasso_even(vertices, edges)
    print(f"v0 can reach a simple cycle with highest priority even: {v0_has_even_cycle}")

    v0_has_odd_cycle = v0_lasso_odd(vertices, edges)
    print(f"All simple cycles reachable from v0 have highest priority even: {v0_has_odd_cycle}")

    # --- Early Termination Condition ---
    if not v0_has_even_cycle or not v0_has_odd_cycle:
        print("Early termination: Either v0 cannot reach a simple cycle with highest priority even "
              "or all simple cycles reachable from v0 have highest priority even.")
        return

    # Prepare log file for this input DOT file
    log_file = f"{os.path.splitext(args.input_file)[0]}_sampled_results.txt"
    total_aggregated = 0
    total_time_ms = 0.0
    games_solved = 0  # Counter for games solved

    with open(log_file, "w") as logf:
        for i in range(args.N):
            bit_string = "".join(random.choice("01") for _ in range(n))

            # Replace players
            replace_players_in_dot(args.input_file, args.output_file, bit_string)

            # Time the solver call
            start_time = time.perf_counter()
            try:
                solver_output = subprocess.check_output([
                    "../ggg/build/solvers/parity/priority_promotion/priority_promotion_solver",
                    "-i", args.output_file,
                    "--csv"
                ], text=True)
            except subprocess.CalledProcessError as e:
                solver_output = f"Solver error: {e}"
            end_time = time.perf_counter()

            elapsed_ms = (end_time - start_time) * 1000
            total_time_ms += elapsed_ms

            # Parse solver CSV output for aggregation
            agg_value = parse_solver_csv(solver_output)
            total_aggregated += agg_value

            # Update counter
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
