#!/usr/bin/env python3
import json
import argparse
import os
import subprocess
import time
import random
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
    game_data = json.loads(json.dumps(base_game))  # deep copy
    for v in game_data["nodes"]:
        vid = v["id"]
        v["owner"] = int(bit_string[vid])
    with open(tmp_filename, "w") as f:
        json.dump(game_data, f, indent=2)


# ----------------------------
# 3️⃣ Parse solver output
# ----------------------------
def parse_solver_output(output):
    for line in output.splitlines():
        line = line.strip()
        if line.startswith("The winning region is:"):
            start = line.find("{")
            end = line.find("}", start)
            if start != -1 and end != -1:
                region_str = line[start:end + 1]
                try:
                    region = eval(region_str)
                    value0 = region.get(0, -1)
                    return 1 if value0 >= 0 else 0
                except Exception:
                    return 0
    return 0


# ----------------------------
# 4️⃣ Worker: run solver for one bitstring
# ----------------------------
def solve_one(bit_string, base_game, tmp_dir):
    tmp_file = os.path.join(tmp_dir, f"game_{bit_string}.json")
    replace_players_in_json(base_game, bit_string, tmp_file)
    try:
        start_time = time.perf_counter()
        solver_output = subprocess.check_output(["egsolver", "solve", tmp_file], text=True)
        elapsed_ms = (time.perf_counter() - start_time) * 1000
        agg_value = parse_solver_output(solver_output)
        return (bit_string, agg_value, elapsed_ms, solver_output)
    except subprocess.CalledProcessError as e:
        return (bit_string, 0, 0.0, f"Solver error: {e}")
    finally:
        # Optional: cleanup to save space
        try:
            os.remove(tmp_file)
        except FileNotFoundError:
            pass


# ----------------------------
# 5️⃣ Main driver with sampling + periodic aggregated logging
# ----------------------------
def main():
    parser = argparse.ArgumentParser(description="Parallel sampled energy-game solver with aggregated progress logging.")
    parser.add_argument("input_file", help="Path to JSON input file.")
    parser.add_argument("--tmpdir", default="tmp_solvers", help="Temporary directory for intermediate JSONs.")
    parser.add_argument("--workers", type=int, default=cpu_count(), help="Number of parallel workers.")
    parser.add_argument("--samples", "-N", type=int, default=100000, help="Number of random bitstrings to sample.")
    parser.add_argument("--seed", type=int, default=42, help="Random seed for reproducibility.")
    parser.add_argument("--interval", type=int, default=1000, help="How often to log aggregated progress.")
    args = parser.parse_args()

    os.makedirs(args.tmpdir, exist_ok=True)
    random.seed(args.seed)
    print(f"Using random seed: {args.seed}")

    vertices, edges, weights, game_data = read_energy_game(args.input_file)
    n = len(vertices)
    total_possible = 2 ** n

    print(f"{n} vertices detected. Sampling {args.samples} / {total_possible} assignments using {args.workers} workers...")

    # ✅ Random unique bitstrings
    bit_strings = set()
    while len(bit_strings) < args.samples:
        bit_strings.add("".join(random.choice("01") for _ in range(n)))
    bit_strings = list(bit_strings)

    log_file = f"{os.path.splitext(args.input_file)[0]}_energy_results_sampled.txt"
    total_aggregated = 0
    total_time_ms = 0.0
    solved = 0

    start_global = time.perf_counter()

    with open(log_file, "w") as logf:
        with Pool(processes=args.workers) as pool:
            worker = partial(solve_one, base_game=game_data, tmp_dir=args.tmpdir)
            for idx, (bit_string, agg_value, elapsed_ms, solver_output) in enumerate(pool.imap_unordered(worker, bit_strings), 1):
                total_aggregated += agg_value
                total_time_ms += elapsed_ms

                # logf.write(f"Bit string: {bit_string}\n")
                # logf.write(solver_output.strip() + "\n")
                # logf.write(f"Winner (1=MAX wins, 0=LOSES): {agg_value}\n")
                # logf.write(f"Time: {elapsed_ms:.3f} ms\n")
                # logf.write("-" * 40 + "\n")

                # every 1000 samples, log progress summary
                if idx % 1000 == 0:
                    elapsed_sec = (time.perf_counter() - start_global)
                    summary = f"{total_aggregated}/{idx}; {total_time_ms:.3f}\n"
                    print(summary.strip())
                    logf.write(summary)
                    logf.flush()

    elapsed_total = (time.perf_counter() - start_global) * 1000
    print(f"\n Finished sampling {args.samples} bitstrings")
    print(f"Total aggregated value: {total_aggregated}/{args.samples}")
    print(f"Total solver time (wall-clock): {elapsed_total:.3f} ms")

    # with open(log_file, "a") as logf:
    #     logf.write(f"\n✅ All results saved to {log_file}\n")
    #     logf.write(f"Total aggregated value: {total_aggregated}/{args.samples}\n")
    #     logf.write(f"Total solver time (wall-clock): {elapsed_total:.3f} ms\n")


if __name__ == "__main__":
    main()
