import argparse
import os
import random

def generate_parity_game(file, vertices, max_priority, min_out_degree, max_out_degree, rng):
    """
    Generate a random parity game in DOT format and write to file.
    """
    file.write("digraph ParityGame {\n")

    # Generate vertices
    for i in range(vertices):
        player = rng.randint(0, 1)
        priority = rng.randint(0, max_priority)
        file.write(f'  v{i} [name="v{i}", player={player}, priority={priority}];\n')

    # Generate edges
    for i in range(vertices):
        out_degree = rng.randint(min_out_degree, max_out_degree)
        targets = list(range(vertices))
        rng.shuffle(targets)
        actual_targets = targets[:min(out_degree, len(targets))]
        for target in actual_targets:
            file.write(f'  v{i} -> v{target} [label="edge_{i}_{target}"];\n')

    file.write("}\n")


def generate_game_file(output_dir, game_index, vertices, max_priority, min_out_degree, max_out_degree, rng):
    filename = f"parity_game_{game_index + 1}.dot"
    file_path = os.path.join(output_dir, filename)
    with open(file_path, "w") as f:
        generate_parity_game(f, vertices, max_priority, min_out_degree, max_out_degree, rng)
    return filename


def main():
    parser = argparse.ArgumentParser(description="Generate random parity games in DOT format.")
    parser.add_argument("-o", "--output-dir", required=True, help="Output directory for generated games")
    parser.add_argument("-n", "--count", type=int, default=10, help="Number of games to generate")
    parser.add_argument("-v", "--vertices", type=int, default=10, help="Number of vertices per game")
    parser.add_argument("--max-priority", type=int, default=5, help="Maximum priority for parity games")
    parser.add_argument("--min-out-degree", type=int, default=1, help="Minimum out-degree for each vertex")
    parser.add_argument("--max-out-degree", type=int, help="Maximum out-degree for each vertex (default: vertices-1)")
    parser.add_argument("-s", "--seed", type=int, help="Random seed (default: random)")
    parser.add_argument("--verbose", action="store_true", help="Show detailed output")

    args = parser.parse_args()

    output_dir = args.output_dir
    count = args.count
    vertices = args.vertices
    max_priority = args.max_priority
    min_out_degree = args.min_out_degree
    max_out_degree = args.max_out_degree if args.max_out_degree is not None else vertices - 1
    verbose = args.verbose
    seed = args.seed if args.seed is not None else random.randrange(0, 2**32)

    # Validate parameters
    if min_out_degree < 1:
        raise ValueError("min-out-degree must be at least 1")
    if max_out_degree < min_out_degree:
        raise ValueError("max-out-degree must be at least min-out-degree")
    if max_out_degree > vertices:
        raise ValueError(f"max-out-degree must be at most number of vertices (max: {vertices})")

    # Create output directory if not exists
    os.makedirs(output_dir, exist_ok=True)

    rng = random.Random(seed)

    if verbose:
        print(f"Generating {count} parity games with {vertices} vertices each")
        print(f"Out-degree range: [{min_out_degree}, {max_out_degree}]")
        print(f"Random seed: {seed}")
        print(f"Output directory: {output_dir}\n")

    for i in range(count):
        filename = generate_game_file(output_dir, i, vertices, max_priority, min_out_degree, max_out_degree, rng)
        if verbose:
            print(f"Generated: {filename}")

    if verbose:
        print(f"\nSuccessfully generated {count} parity games")


if __name__ == "__main__":
    main()
