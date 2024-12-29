#!/bin/bash

# Check if a source file was provided as an argument
if [ -z "$1" ]; then
    echo "Usage: $0 <source_file.c>"
    exit 1
fi

# Compile the source file with gcc
gcc "$1" -o "hello"
if [ $? -ne 0 ]; then
    echo "Compilation failed."
    exit 1
fi

echo "Compilation successful. Executable created: ./main"
