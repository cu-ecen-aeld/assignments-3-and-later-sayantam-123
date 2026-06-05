#!/bin/bash

writefile=$1
writestr=$2

# Check arguments
if [ $# -ne 2 ]; then
    echo "Error: Two arguments required."
    exit 1
fi

# Create parent directories if they do not exist
mkdir -p "$(dirname "$writefile")" || {
    echo "Error: Could not create directory path."
    exit 1
}

# Create/overwrite file with content
echo "$writestr" > "$writefile" || {
    echo "Error: Could not create file."
    exit 1
}
