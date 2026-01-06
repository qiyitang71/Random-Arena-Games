#!/bin/bash

# Folder containing the .dot files
FOLDER="Parity"

# Temporary output DOT file
TMP_DOT="$FOLDER/tmp.dot"

# Loop over all .dot files in the folder
for dot_file in "$FOLDER"/*.dot; do
    # Skip if no .dot files exist
    [ -e "$dot_file" ] || continue

    echo "Running compute-exact-probability-parity.py for $dot_file ..."
    python compute-exact-probability-parity.py "$dot_file" "$TMP_DOT"
done

echo "All Parity/*.dot files processed."
