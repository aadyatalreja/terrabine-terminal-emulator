#!/bin/bash

if [ "$#" -ne 2 ]; then
  echo "Usage: mv <source> <destination>"
  exit 1
fi

SOURCE="$1"
DESTINATION="$2"

if [ ! -e "$SOURCE" ]; then
  echo "mv: $SOURCE: No such file or directory"
  exit 1
fi

/bin/mv "$SOURCE" "$DESTINATION"
