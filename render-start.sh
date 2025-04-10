#!/bin/bash
# Navigate to server directory
cd server || exit 1

# Ensure Makefile exists
if [ ! -f "Makefile" ]; then
    echo "Error: Makefile not found in server directory!"
    exit 1
fi

# Build the server
make

# Make sure the server binary exists
if [ ! -f "server" ]; then
    echo "Error: Server binary not found after build!"
    exit 1
fi

# Set execute permission (just in case)
chmod +x server

# Start the server
./server