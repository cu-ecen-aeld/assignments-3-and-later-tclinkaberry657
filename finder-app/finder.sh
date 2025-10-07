#!/bin/bash

if [ $# -ne 2 ]
then
    echo "Incorrect number of arguments specified.
    usage: finder.sh <filesdir> <searchstr>"
    exit 1
fi

if [ -d "$1" ]
then
    file_count=$(grep -rlI "$2" "$1" | wc -l)
    line_count=$(grep -rI "$2" "$1" | wc -l)
    echo "The number of files are ${file_count} and the number of matching lines are ${line_count}"
else
    echo "<filesdir> not a valid directory."
    exit 1
fi
