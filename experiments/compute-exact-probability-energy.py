#!/usr/bin/env python3
import json
import argparse
import itertools
import os
import subprocess
import time

# ----------------------------
# 1️⃣ Read JSON energy game
# ----------------------------
def read_energy_game(json_file):
    with open(json_file, "r") as f:
        game_data = json.load(f)

    vertices = {v["id"]: v["owner"] for v in game_data["nodes"]}
    edges = {}
    weights = {}
    for e in game_data["edges"]:
        src = e["source"]
        tgt = e["target"]
        effect = e.get("effect", 0)
        edges.setdefault(src, []).append(tgt)
        weights[(src, tgt)] = effect

    return vertices, edges, weights, game_data

# ----------------------------
# 2️⃣ Replace player assignments in JSON
# ----------------------------
def replace_players_in_json(game_data, bit_string, output_file):
    for v in game_data["nodes"]:
        vid = v["id"]
        v["owner"] = int(bit_string[vid])
    with open(output_file, "w") as f:
        json.dump(game_data, f, indent=2)

# ----------------------------
# 3️⃣ Find a nonnegative-energy lasso from start
# ----------------------------
def find_nonnegative_lasso(edges, weights, start=0, energy_limit=1000):
    stack = [(start, [start], 0)]
    visited_states = set()

    while stack:
        current, path, energy = stack.pop()
        if energy < 0:
            continue

        if current in path[:-1]:
            cycle_start = path.index(current)
            cycle = path[cycle_start:]
            total_weight = 0
            valid = True
            cycle_energy = 0
            for i in range(len(cycle) - 1):
                w = weights.get((cycle[i], cycle[i + 1]), 0)
                cycle_energy += w
                if cycle_energy < 0:
                    valid = False
                    break
                total_weight += w
            if valid and total_weight >= 0:
                return True
            continue

        for neighbor in edges.get(current, []):
            w = weights.get((current, neighbor), 0)
            new_energy = energy + w
            state = (neighbor, new_energy)
            if new_energy >= 0 and state not in visited_states and new_energy <= energy_limit:
                visited_states.add(state)
                stack.append((neighbor, path + [neighbor], new_energy))

    return False

# ----------------------------
# 4️⃣ Detect any negative-energy path
# ----------------------------
def exists_negative_energy_path(edges, weights, start=0, energy_limit=1000):
    stack = [(start, 0)]
    visited_states = set()

    while stack:
        current, energy = stack.pop()
        if (current, energy) in visited_states:
            continue
        visited_states.add((current, energy))

        for neighbor in edges.get(current, []):
            w = weights.get((current, neighbor), 0)
            new_energy = energy + w
            if new_energy < 0:
                return True
            if new_energy <= energy_limit:
                stack.append((neighbor, new_energy))

    return False

# ----------------------------
# Parse solver output (updated)
# ----------------------------
def parse_solver_output(output):
    """
    Scan solver output lines for "The winning region is: {...}".
    Extract the number corresponding to key 0 and return:
      - 1 if value >= 0
      - 0 if value < 0
    """
    for line in output.splitlines():
        line = line.strip()
        if line.startswith("The winning region is:"):
            # extract the dict part
            start = line.find("{")
            end = line.find("}", start)
            if start != -1 and end != -1:
                region_str = line[start:end+1]
                try:
                    region = eval(region_str)  # convert string to dict
                    value0 = region.get(0, -1)
                    return 1 if value0 >= 0 else 0
                except Exception as e:
                    print(f"Error parsing winning region: {e}")
                    return 0
    # fallback
    return 0


# ----------------------------
# 6️⃣ Main experiment
# ----------------------------
def main():
    parser = argparse.ArgumentParser(description="Analyze Energy Game JSON and enumerate player assignments.")
    parser.add_argument("input_file", help="Path to the JSON input file.")
    parser.add_argument("output_file", help="Path to save temporary JSON with player assignments.")
    args = parser.parse_args()

    vertices, edges, weights, game_data = read_energy_game(args.input_file)

    # Early checks
    v0_can_win = find_nonnegative_lasso(edges, weights, start=0)
    v0_has_neg_path = exists_negative_energy_path(edges, weights, start=0)

    print(f"v0 has nonnegative-energy lasso: {v0_can_win}")
    print(f"v0 can reach a negative-energy path: {v0_has_neg_path}")

    if not v0_can_win or not v0_has_neg_path:
        print("Early termination: graph unsuitable for exhaustive analysis.")
        return

    n = len(vertices)
    total_games = 2 ** n
    print(f"{n} vertices detected, enumerating all {total_games} player assignments...")

    log_file = f"{os.path.splitext(args.input_file)[0]}_energy_results.txt"
    total_aggregated = 0
    total_time_ms = 0.0
    games_solved = 0

    with open(log_file, "w") as logf:
        for bits in itertools.product("01", repeat=n):
            bit_string = "".join(bits)
            replace_players_in_json(game_data, bit_string, args.output_file)

            start_time = time.perf_counter()
            try:
                solver_output = subprocess.check_output(
                    ["egsolver", "solve", args.output_file],
                    text=True
                )
            except subprocess.CalledProcessError as e:
                solver_output = e.output or f"Solver error: {e}"
            end_time = time.perf_counter()

            elapsed_ms = (end_time - start_time) * 1000
            total_time_ms += elapsed_ms

            agg_value = parse_solver_output(solver_output)
            total_aggregated += agg_value
            games_solved += 1

            # logf.write(f"Bit string: {bit_string}\n")
            # logf.write(solver_output.strip() + "\n")
            # logf.write(f"Python measured time: {elapsed_ms:.3f} ms\n")
            # logf.write(f"Winner (1=MAX wins, 0=LOSES): {agg_value}\n")
            # logf.write(f"Aggregated: {total_aggregated}/{games_solved}\n")
            # logf.write("-" * 40 + "\n")
            # logf.flush()

            if games_solved % 1000 == 0:
                print(f"Solved {games_solved}/{total_games} | Agg: {total_aggregated} | Time: {elapsed_ms:.3f} ms")
        logf.write(f"{total_aggregated}/{games_solved}")

    print(f"\n All results saved to {log_file}")
    print(f"Total games solved: {games_solved}/{total_games}")
    print(f"Total aggregated value: {total_aggregated}")
    print(f"Total solver time: {total_time_ms:.3f} ms")
    if games_solved > 0:
        print(f"Average per game: {total_time_ms / games_solved:.3f} ms")
    else:
        print("Average per game: N/A")

if __name__ == "__main__":
    main()

