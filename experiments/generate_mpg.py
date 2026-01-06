#!/usr/bin/env python3
import argparse
import os
import random
import sys

class MPGameGenerator:
    @staticmethod
    def run():
        parser = argparse.ArgumentParser(
            description="Mean Payoff Game Generator Options"
        )
        parser.add_argument(
            "-o", "--output-dir",
            required=True,
            help="Output directory for generated games"
        )
        parser.add_argument(
            "-n", "--count",
            type=int, default=10,
            help="Number of games to generate (default: 10)"
        )
        parser.add_argument(
            "-v", "--vertices",
            type=int, default=10,
            help="Number of vertices per game (default: 10)"
        )
        parser.add_argument(
            "--max-weight",
            type=int, default=5,
            help="Maximum weight for mean payoff games (default: 5)"
        )
        parser.add_argument(
            "--min-out-degree",
            type=int, default=1,
            help="Minimum out-degree for each vertex (default: 1)"
        )
        parser.add_argument(
            "--max-out-degree",
            type=int,
            help="Maximum out-degree for each vertex (default: vertices-1)"
        )
        parser.add_argument(
            "-s", "--seed",
            type=int,
            help="Random seed (default: random)"
        )
        parser.add_argument(
            "--verbose",
            action="store_true",
            help="Show detailed output"
        )

        args = parser.parse_args()

        # Validate parameters
        max_out_degree = args.max_out_degree if args.max_out_degree is not None else args.vertices - 1
        if args.min_out_degree < 1:
            print("Error: min-out-degree must be at least 1", file=sys.stderr)
            sys.exit(1)
        if max_out_degree < args.min_out_degree:
            print("Error: max-out-degree must be at least min-out-degree", file=sys.stderr)
            sys.exit(1)
        if max_out_degree > args.vertices:
            print(f"Error: max-out-degree must be at most number of vertices (max: {args.vertices})", file=sys.stderr)
            sys.exit(1)

        # Set random seed
        seed = args.seed if args.seed is not None else random.randint(0, 2**32 - 1)
        random.seed(seed)

        # Generate games
        MPGameGenerator.generate_games(
            args.output_dir,
            args.count,
            args.vertices,
            args.max_weight,
            args.min_out_degree,
            max_out_degree,
            seed,
            args.verbose
        )

    @staticmethod
    def generate_games(output_dir, count, vertices, max_weight, min_out_degree, max_out_degree, seed, verbose):
        # Create output directory
        os.makedirs(output_dir, exist_ok=True)

        if verbose:
            print(f"Generating {count} mean-payoff games with {vertices} vertices each")
            print(f"Out-degree range: [{min_out_degree}, {max_out_degree}]")
            print(f"Random seed: {seed}")
            print(f"Output directory: {output_dir}")
            print()

        for i in range(count):
            filename = MPGameGenerator.generate_game_file(
                output_dir, i, vertices, max_weight, min_out_degree, max_out_degree
            )
            if verbose:
                print(f"Generated: {filename}")

        if verbose:
            print()
            print(f"Successfully generated {count} mean-payoff games")

    @staticmethod
    def generate_game_file(output_dir, game_index, vertices, max_weight, min_out_degree, max_out_degree):
        filename = f"mp_game_{game_index + 1}.dot"
        file_path = os.path.join(output_dir, filename)

        with open(file_path, "w") as f:
            MPGameGenerator.generate_mp_game(f, vertices, max_weight, min_out_degree, max_out_degree)

        return filename

    @staticmethod
    def generate_mp_game(file, vertices, max_weight, min_out_degree, max_out_degree):
        file.write("digraph MPGame {\n")

        # Generate vertices
        for i in range(vertices):
            player = random.randint(0, 1)
            weight = random.randint(-max_weight, max_weight)
            file.write(f'  v{i} [name="v{i}", player={player}, weight={weight}];\n')

        # Generate edges with controlled out-degrees
        for i in range(vertices):
            out_degree = random.randint(min_out_degree, max_out_degree)
            targets = list(range(vertices))
            random.shuffle(targets)
            for k in range(out_degree):
                target = targets[k]
                file.write(f'  v{i} -> v{target} [label="edge_{i}_{target}"];\n')

        file.write("}\n")


if __name__ == "__main__":
    MPGameGenerator.run()
