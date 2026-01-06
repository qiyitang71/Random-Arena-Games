#!/usr/bin/env python3
import json
import argparse
import itertools
import os
import subprocess
import time
from multiprocessing import Pool, cpu_count
from functools import partial

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
def replace_players_in_json(base_game, bit_string, tmp_filename):
    """Write a new JSON with owners replaced according to bit_string."""
    game_data = json.loads(json.dumps(base_game))  # deep copy
    for v in game_data["nodes"]:
        vid = v["id"]
        v["owner"] = int(bit_string[vid])
    with open(tmp_filename, "w") as f:
        json.dump(game_data, f, indent=2)


# ----------------------------
# 3️⃣ Find a nonnegative-energy lasso
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
# 5️⃣ Parse solver output
# ----------------------------
def parse_solver_output(output):
    for line in output.splitlines():
        line = line.strip()
        if line.startswith("The winning region is:"):
            start = line.find("{")
            end = line.find("}", start)
            if start != -1 and end != -1:
                region_str = line[start:end+1]
                try:
                    region = eval(region_str)
                    value0 = region.get(0, -1)
                    return 1 if value0 >= 0 else 0
                except Exception:
                    return 0
    return 0


# ----------------------------
# 6️⃣ Worker function (runs in parallel)
# ----------------------------
def solve_one(bit_string, base_game, tmp_dir):
    tmp_file = os.path.join(tmp_dir, f"game_{bit_string}.json")
    replace_players_in_json(base_game, bit_string, tmp_file)

    try:
        start_time = time.perf_counter()
        solver_output = subprocess.check_output(
            ["egsolver", "solve", tmp_file],
            text=True
        )
        elapsed_ms = (time.perf_counter() - start_time) * 1000
        agg_value = parse_solver_output(solver_output)
        return (bit_string, agg_value, elapsed_ms, solver_output)
    except subprocess.CalledProcessError as e:
        return (bit_string, 0, 0.0, f"Solver error: {e}")


# ----------------------------
# 7️⃣ Main driver
# ----------------------------
def main():
    parser = argparse.ArgumentParser(description="Parallel energy-game solver over all player assignments.")
    parser.add_argument("input_file", help="Path to the JSON input file.")
    parser.add_argument("--tmpdir", default="tmp_solvers", help="Temporary directory for intermediate JSONs.")
    parser.add_argument("--workers", type=int, default=cpu_count(), help="Number of parallel worker processes.")
    args = parser.parse_args()

    os.makedirs(args.tmpdir, exist_ok=True)

    vertices, edges, weights, game_data = read_energy_game(args.input_file)

    v0_can_win = find_nonnegative_lasso(edges, weights)
    v0_has_neg_path = exists_negative_energy_path(edges, weights)

    print(f"v0 has nonnegative-energy lasso: {v0_can_win}")
    print(f"v0 can reach a negative-energy path: {v0_has_neg_path}")

    if not v0_can_win or not v0_has_neg_path:
        print("Early termination: graph unsuitable for exhaustive analysis.")
        return

    n = len(vertices)
    total_games = 2 ** n
    print(f"Enumerating {total_games} player assignments using {args.workers} cores...")

    bit_strings = ["".join(bits) for bits in itertools.product("01", repeat=n)]

    start_global = time.perf_counter()
    total_aggregated = 0
    total_time_ms = 0.0

    log_file = f"{os.path.splitext(args.input_file)[0]}_energy_results.txt"
    with open(log_file, "w") as logf:
        with Pool(processes=args.workers) as pool:
            worker = partial(solve_one, base_game=game_data, tmp_dir=args.tmpdir)
            for bit_string, agg_value, elapsed_ms, solver_output in pool.imap_unordered(worker, bit_strings):
                total_aggregated += agg_value
                total_time_ms += elapsed_ms
                logf.write(f"Bit string: {bit_string}\n")
                logf.write(solver_output.strip() + "\n")
                logf.write(f"Winner (1=MAX wins, 0=LOSES): {agg_value}\n")
                logf.write(f"Time: {elapsed_ms:.3f} ms\n")
                logf.write("-" * 40 + "\n")
                logf.flush()

    elapsed_total = (time.perf_counter() - start_global) * 1000
    print(f"\n✅ All results saved to {log_file}")
    print(f"Total aggregated value: {total_aggregated}")
    print(f"Total solver time (wall-clock): {elapsed_total:.3f} ms using {args.workers} workers")


if __name__ == "__main__":
    main()
