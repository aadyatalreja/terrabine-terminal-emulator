#!/bin/bash

#check if directory was provided in arg
if [ "$1" ]; then
  DIRECTORY="$1"
else
  DIRECTORY="."
fi 

if [ ! -d "$DIRECTORY" ]; then #-d means directory
  echo "ls: $DIRECTORY: No such directory"
  exit 1 
fi 

#to list contents of directory

for entry in "$DIRECTORY"/*; do 
  if [ -d "$entry" ]; then
    echo "[DIR] $(basename "$entry")"

  elif [ -f "$entry" ]; then
    echo "[FILE] $(basename "$entry")"

  else
    echo "[OTHER] $(basename "$entry")"
  fi 
done




