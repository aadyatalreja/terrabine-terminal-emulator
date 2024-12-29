#!/bin/bash

if [ "$1" ]; then
  FILE="$1"
else
  echo "delete: missing operand"
  exit 1 
fi 

if [ -f "$FILE" ]; then
  rm "$FILE"
  echo "File '$FILE' deleted."
else
  echo "delete: $FILE: No such file"
  exit 1
fi
