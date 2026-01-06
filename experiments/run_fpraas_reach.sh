#!/bin/bash

# Folder containing .dot and results.txt files
FOLDER="Reach"

# Temporary output file
TMP_DOT="$FOLDER/tmp.dot"

# Number of samples
N=100000

# Loop over all .dot files in the folder
for dot_file in "$FOLDER"/*.dot; do
    # Extract the base filename without extension
    base_name=$(basename "$dot_file" .dot)

    # Expected results file
    results_file="$FOLDER/${base_name}_results.txt"

    # Check if results file exists
    if [ -f "$results_file" ]; then
        echo "Running fpraas-reach.py for $dot_file (because $results_file exists)"
        python fpraas-reach.py "$dot_file" "$TMP_DOT" "$N"
    else
        echo "Skipping $dot_file (no $results_file found)"
    fi
done

echo "All eligible .dot files processed."
