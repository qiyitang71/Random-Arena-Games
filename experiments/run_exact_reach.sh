#!/bin/bash

# Folder containing the .dot files
FOLDER="Reach"

# Temporary output DOT file
TMP_DOT="$FOLDER/tmp.dot"

# Loop over all .dot files in the folder
for dot_file in "$FOLDER"/*.dot; do
    # Skip if no .dot files exist
    [ -e "$dot_file" ] || continue

    echo "Running compute-exact-probability-reach.py for $dot_file ..."
    python compute-exact-probability-reach.py "$dot_file" "$TMP_DOT"
done

echo "All $FOLDER/*.dot files processed."
