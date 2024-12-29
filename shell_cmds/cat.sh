#!/bin/bash

if [ "$#" -eq 0 ]; then
  echo "cat: missing file operand"
  exit 1
fi

for FILE in "$@"; do
  if [ -f "$FILE" ]; then
    cat "$FILE"
  else
    echo "cat: $FILE: No such file"
  fi
done
