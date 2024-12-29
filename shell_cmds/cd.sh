#!/bin/bash

# If no argument is provided, go to home directory
if [ $# -eq 0 ]; then
    cd "$HOME" 2>/dev/null
    pwd
    exit 0
fi

# Handle special case of "-" (previous directory)
if [ "$1" = "-" ]; then
    cd - 2>/dev/null
    pwd
    exit 0
fi

# Try to change to the specified directory
if cd "$1" 2>/dev/null; then
    pwd
    exit 0
else
    echo "Directory '$1' does not exist" >&2
    exit 1
fi
