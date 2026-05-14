#!/bin/sh

# Simple build script for the VCS emulator
# Requires: gcc, make

echo "Building VCS Emulator..."
make

if [ $? -eq 0 ]; then
    echo "Build successful! Executable is at dist/vcs"
else
    echo "Build failed."
    exit 1
fi
