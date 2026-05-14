#!/bin/sh

# Simple run script for the VCS emulator

if [ ! -f "dist/vcs" ]; then
    echo "Executable dist/vcs not found. Please build it first with ./build.sh"
    exit 1
fi

if [ $# -lt 1 ]; then
    echo "Usage: $0 file.rom"
    exit 1
fi

./dist/vcs "$@"
