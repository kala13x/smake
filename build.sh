#!/bin/bash
make clean
make

if [ "$?" = "0" ]; then
    echo "Build done"
else
    echo "Build failed"
fi