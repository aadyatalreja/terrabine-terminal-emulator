#!/bin/bash

# Check if a filename is provided
if [ -z "$1" ]; then
  echo "Usage: $0 <filename>"
  exit 1
fi

# Create the file
touch "$1"

# Check if the file was created successfully
if [ -e "$1" ]; then
  echo "File '$1' has been created successfully."
else
  echo "Failed to create file '$1'."
fi
