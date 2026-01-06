#!/bin/bash

# Folder containing the .dot files
FOLDER="MPGames2"

# Temporary output DOT file
TMP_DOT="$FOLDER/tmp.dot"

# Loop over all .dot files in the folder
for dot_file in "$FOLDER"/*.dot; do
    # Skip if no .dot files exist
    [ -e "$dot_file" ] || continue

    echo "Running compute-exact-probability-energy.py for $dot_file ..."
    python compute-exact-probability-energy.py "$dot_file" "$TMP_DOT"
done

echo "All MPGames2/*.dot files processed."
