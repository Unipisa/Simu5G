#!/bin/bash

# move *.UPDATED files
for FILE in *.UPDATED; do
    mv "$FILE" "${FILE%.UPDATED}" 
done

# remove all .FAILED files 
for FILE in *.FAILED; do
    rm "$FILE" 
done
